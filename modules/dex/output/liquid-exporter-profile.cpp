// Copyright (C) 2020 Vincent Chambrin
// This file is part of the 'dex' project
// For conditions of distribution and use, see copyright notice in LICENSE

#include "dex/output/liquid-exporter-profile.h"

#include "dex/common/errors.h"
#include "dex/common/file-utils.h"
#include "dex/common/settings.h"
#include "dex/common/string-utils.h"

#include <QDirIterator>
#include <QFileInfo>

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

  void read_template(const std::string& name, LiquidExporterProfile::Template& tmplt, std::string default_out)
  {
    std::string path = dex::settings::read(settings, "templates/" + name, std::string());

    if (!path.empty())
    {
      path = directory.absolutePath().toStdString() + "/" + path;
      exclude(path);
      tmplt.model = open_liquid_template(path);
      tmplt.model.skipWhitespacesAfterTag();
      tmplt.outdir = dex::settings::read(settings, "output/" + name, std::move(default_out));
      tmplt.filesuffix = QFileInfo(directory.absolutePath() + "/" + QString::fromStdString(path)).suffix().toStdString();
    }
  }

  void list_templates()
  {
    std::string pathlist = dex::settings::read(settings, "templates/others", std::string());

    std::vector<std::string> paths = dex::str_split(pathlist, ',');

    for (std::string p : paths)
    {
      p = directory.absolutePath().toStdString() + "/" + p;
      exclude(p);
      liquid::Template tmplt = open_liquid_template(p);
      tmplt.skipWhitespacesAfterTag();
      p.erase(p.begin(), p.begin() + profile.profile_path.length() + 1);
      profile.liquid_templates.emplace_back(std::move(p), std::move(tmplt));
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
      tmplt.skipWhitespacesAfterTag();

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

    read_template("class", profile.class_template, "classes");
    read_template("namespace", profile.namespace_template, "namespaces");
    read_template("document", profile.document_template, "documents");

    list_templates();
    list_files();
  }
};

void LiquidExporterProfile::load(const QDir& dir)
{
  LiquidExporterProfileLoader loader{ *this, dir };
  loader.load();
}

} // namespace dex
