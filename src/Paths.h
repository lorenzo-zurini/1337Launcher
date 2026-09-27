#pragma once

#include <QSettings>
#include <QString>

namespace Paths {

// Must be called once after QApplication is constructed.
void init();

QString data();
QString instances();
QString libraries();
QString assets();
QString versions();
QString runtimes();

QSettings &settings();

} // namespace Paths
