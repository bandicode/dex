// Copyright (C) 2019 Vincent Chambrin
// This file is part of the 'dex' project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <QTest>

class TestDexOutput : public QObject
{
  Q_OBJECT
public:
  TestDexOutput() = default;

private Q_SLOTS:
  void jsonExport();
  void jsonExportManual();
  void markdownExport();
  void markdownExportManual();
};
