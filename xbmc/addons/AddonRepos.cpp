/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonRepos.h"

#include "Addon.h"
#include "AddonDatabase.h"
#include "AddonManager.h"
#include "AddonSystemSettings.h"
#include "CompileInfo.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <vector>

using namespace ADDON;

namespace ADDON
{

std::vector<RepoInfo> LoadOfficialRepoInfos()
{
  const std::vector<std::string> officialAddonRepos =
      StringUtils::Split(CCompileInfo::GetOfficialAddonRepos(), ',');

  std::vector<RepoInfo> officialRepoInfos;
  RepoInfo newRepoInfo;

  for (const auto& addonRepo : officialAddonRepos)
  {
    const std::vector<std::string> tmpRepoInfo = StringUtils::Split(addonRepo, '|');
    newRepoInfo.m_repoId = tmpRepoInfo.front();
    newRepoInfo.m_origin = tmpRepoInfo.back();
    officialRepoInfos.emplace_back(newRepoInfo);
  }

  return officialRepoInfos;
}

static std::vector<RepoInfo> officialRepoInfos = LoadOfficialRepoInfos();

} // namespace ADDON

/**********************************************************
 * CAddonRepos
 *
 */

bool CAddonRepos::IsFromOfficialRepo(const std::shared_ptr<IAddon>& addon)
{
  return IsFromOfficialRepo(addon, false);
}

bool CAddonRepos::IsFromOfficialRepo(const std::shared_ptr<IAddon>& addon, bool bCheckAddonPath)
{
  auto comparator = [&](const RepoInfo& officialRepo) {
    if (bCheckAddonPath)
    {
      return (addon->Origin() == officialRepo.m_repoId &&
              StringUtils::StartsWithNoCase(addon->Path(), officialRepo.m_origin));
    }

    return addon->Origin() == officialRepo.m_repoId;
  };

  return addon->Origin() == ORIGIN_SYSTEM ||
         std::any_of(officialRepoInfos.begin(), officialRepoInfos.end(), comparator);
}

void CAddonRepos::LoadAddonsFromDatabase(const CAddonDatabase& database)
{
  LoadAddonsFromDatabase(database, "");
}

void CAddonRepos::LoadAddonsFromDatabase(const CAddonDatabase& database, const std::string& addonId)
{
  m_allAddons.clear();

  if (addonId.empty())
  {
    // load full repository content
    database.GetRepositoryContent(m_allAddons);
  }
  else
  {
    // load specific addonId only
    database.FindByAddonId(addonId, m_allAddons);
  }

  m_addonsByRepoMap.clear();
  for (const auto& addon : m_allAddons)
  {
    if (m_addonMgr.IsCompatible(*addon))
    {
      m_addonsByRepoMap[addon->Origin()].insert({addon->ID(), addon});
    }
  }

  for (const auto& map : m_addonsByRepoMap)
    CLog::Log(LOGDEBUG, "ADDONS: repo: {} - {} addon(s) loaded", map.first, map.second.size());

  SetupLatestVersionMaps();
}

void CAddonRepos::SetupLatestVersionMaps()
{
  m_latestOfficialVersions.clear();
  m_latestPrivateVersions.clear();
  m_latestVersionsByRepo.clear();

  for (const auto& repo : m_addonsByRepoMap)
  {
    const auto& addonsPerRepo = repo.second;

    for (const auto& addonMapEntry : addonsPerRepo)
    {
      const auto& addonToAdd = addonMapEntry.second;

      if (IsFromOfficialRepo(addonToAdd, true))
      {
        AddAddonIfLatest(addonToAdd, m_latestOfficialVersions);
      }
      else
      {
        AddAddonIfLatest(addonToAdd, m_latestPrivateVersions);
      }

      // add to latestVersionsByRepo
      AddAddonIfLatest(repo.first, addonToAdd, m_latestVersionsByRepo);
    }
  }
}

void CAddonRepos::AddAddonIfLatest(const std::shared_ptr<IAddon>& addonToAdd,
                                   std::map<std::string, std::shared_ptr<IAddon>>& map) const
{
  const auto& latestKnown = map.find(addonToAdd->ID());
  if (latestKnown == map.end() || addonToAdd->Version() > latestKnown->second->Version())
    map[addonToAdd->ID()] = addonToAdd;
}

void CAddonRepos::AddAddonIfLatest(
    const std::string& repoId,
    const std::shared_ptr<IAddon>& addonToAdd,
    std::map<std::string, std::map<std::string, std::shared_ptr<IAddon>>>& map) const
{
  const auto& latestVersionByRepo = map.find(repoId);

  if (latestVersionByRepo == map.end()) // repo not found
  {
    map[repoId].insert({addonToAdd->ID(), addonToAdd});
  }
  else
  {
    const auto& latestVersionEntryByRepo = latestVersionByRepo->second;
    const auto& latestKnown = latestVersionEntryByRepo.find(addonToAdd->ID());

    if (latestKnown == latestVersionEntryByRepo.end() ||
        addonToAdd->Version() > latestKnown->second->Version())
      map[repoId][addonToAdd->ID()] = addonToAdd;
  }
}

void CAddonRepos::BuildUpdateList(const std::vector<std::shared_ptr<IAddon>>& installed,
                                  std::vector<std::shared_ptr<IAddon>>& updates) const
{
  std::shared_ptr<IAddon> update;

  CLog::Log(LOGDEBUG, "ADDONS: *** building update list (installed add-ons) ***");

  for (const auto& addon : installed)
  {
    if (DoAddonUpdateCheck(addon, update))
    {
      updates.emplace_back(update);
    }
  }
}

bool CAddonRepos::DoAddonUpdateCheck(const std::shared_ptr<IAddon>& addon,
                                     std::shared_ptr<IAddon>& update) const
{
  CLog::Log(LOGDEBUG, "ADDONS: update check: addonID = {} / Origin = {}", addon->ID(),
            addon->Origin());

  update.reset();

  if (ORIGIN_SYSTEM == addon->Origin())
  {
    if (!FindAddonAndCheckForUpdate(addon, m_latestOfficialVersions, update))
      return false;
  }
  else
  {
    // if the addon is not found in an official repo...
    if (!FindAddonAndCheckForUpdate(addon, m_latestOfficialVersions, update))
    {
      // ...we move on and check the private/3rd party repo(s)
      if (!FindAddonAndCheckForUpdate(addon, m_latestPrivateVersions, update))
        return false;
    }
  }

  if (update != nullptr)
  {
    CLog::Log(LOGDEBUG, "ADDONS: -- found -->: addonID = {} / Origin = {} / Version = {}",
              update->ID(), update->Origin(), update->Version().asString());
    return true;
  }

  return false;
}

bool CAddonRepos::FindAddonAndCheckForUpdate(
    const std::shared_ptr<IAddon>& addonToCheck,
    const std::map<std::string, std::shared_ptr<IAddon>>& map,
    std::shared_ptr<IAddon>& update) const
{
  const auto& remote = map.find(addonToCheck->ID());
  if (remote != map.end()) // is addon in the desired map?
  {
    if ((remote->second->Version() > addonToCheck->Version()) ||
        m_addonMgr.IsAddonDisabledWithReason(addonToCheck->ID(), AddonDisabledReason::INCOMPATIBLE))
    {
      // return addon update
      update = remote->second;
    }
    else
    {
      // addon found, but it's up to date
      update = nullptr;
    }
    return true;
  }

  return false;
}
