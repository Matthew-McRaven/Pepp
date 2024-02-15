#pragma once

#include <QtQml/QQmlExtensionPlugin>

class Preferences : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "preferences/0.1")
public:
    void registerTypes(const char *uri) override;
};
