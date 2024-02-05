#pragma once

#include <QtQml/QQmlExtensionPlugin>

class Memory : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "memory/0.2")
public:
    void registerTypes(const char *uri) override;
};
