// Copyright (C) 2020 Vincent Chambrin
// This file is part of the 'dex' project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/output/liquid-exporter-profile.h"

#include "dex/common/errors.h"
#include "dex/common/file-utils.h"
#include "dex/common/settings.h"

#include <QDirIterator>

#include <set>

namespace dex
{

static liquid::Template open_liquid_template(const std::string& path)
{
  std::string tmplt = file_utils::read_all(path);
  return liquid::parse(tmplt);
}

class LiquidExporterProfileLoader
{
public:
  LiquidExporterProfile& profile;
  QDir directory;
  std::set<std::string> exclusions;
  SettingsMap settings;

public:
  LiquidExporterProfileLoader(LiquidExporterProfile& pro, const QDir& dir)
    : profile(pro),
      directory(dir)
  {

  }

protected:
  bool excluded(const std::string& filepath)
  {
    return exclusions.find(filepath) != exclusions.end();
  }

  void exclude(std::string filepath)
  {
    exclusions.insert(std::move(filepath));
  }

  void exclude(const QString& filepath)
  {
    exclude(filepath.toStdString());
  }

  void read_template(const std::string& name, liquid::Template& src, std::string& dest, std::string default_out)
  {
    std::string path = dex::settings::read(settings, "templates/" + name, std::string());

    if (!path.empty())
    {
      path = directory.absolutePath().toStdString() + "/" + path;
      exclude(path);
      src = open_liquid_template(path);

      dest = dex::settings::read(settings, "output/" + name, std::move(default_out));
    }
  }

  void list_files()
  {
    QDirIterator diriterator{ directory.absolutePath(), QDir::NoDotAndDotDot | QDir::Files, QDirIterator::Subdirectories };

    while (diriterator.hasNext())
    {
      std::string path = diriterator.next().toStdString();

      if (excluded(path))
        continue;

      liquid::Template tmplt = open_liquid_template(path);

      path.erase(path.begin(), path.begin() + profile.profile_path.length() + 1);
      profile.files.emplace_back(std::move(path), std::move(tmplt));
    }
  }

public:

  void load()
  {
    if (!directory.exists("config.ini"))
      throw std::runtime_error{ "Bad profile directory" };

    profile.profile_path = directory.absolutePath().toStdString();

    std::string profile_config_file = directory.absoluteFilePath("config.ini").toStdString();
    exclude(profile_config_file);
    settings = dex::settings::load(profile_config_file);

    read_template("class", profile.class_template, profile.class_outdir, "classes");

    list_files();
  }
};

void LiquidExporterProfile::load(const QDir& dir)
{
  LiquidExporterProfileLoader loader{ *this, dir };
  loader.load();
}

} // namespace dex
