function Component() {
    if (installer.isInstaller() && installer.value("os") === "win")
        installer.addWizardPage(component, "ShortcutPage", QInstaller.ReadyForInstallation)
}

Component.prototype.createOperations = function () {
    component.createOperations()
    if (installer.value("os") === "win") {
        installer.setValue("RunProgram", "@TargetDir@/bin/@ProductName@.exe")
        //win -> add startmenu shortcuts
        component.addOperation("CreateShortcut",
                               "@TargetDir@/bin/@ProductName@.exe",
                               "@StartMenuDir@/@Name@.lnk")
        if (installer.isOfflineOnly())
            component.addOperation("CreateShortcut",
                                   "@TargetDir@/@MaintenanceToolName@.exe",
                                   "@StartMenuDir@/Uninstall Pepp.lnk")
        else {
            component.addOperation("CreateShortcut",
                                   "@TargetDir@/@MaintenanceToolName@.exe",
                                   "@StartMenuDir@/@MaintenanceToolName@.lnk")
        }

        var targetDir = installer.value("TargetDir").replace(/\//g, "\\"); // Must normalize to windows \ from internal /

        // Register associations
        var extensions = ["pep", "pepo", "pepcpu", "pepm"]
        for (var i = 0; i < extensions.length; ++i) {
            var ext = extensions[i]

            component.addOperation(
                          "RegisterFileType", ext,
                          "\"" +targetDir + "\\bin\\@ProductName@.exe\" gui \"%1\"",
                          "Pepp IDE file type", "text/plain",
                          // 0 Should be the app's default icon?
                          "@TargetDir@/bin/@ProductName@.exe,0",
                          "ProgId=edu.pepperdine.Pepp." + ext)
        }

        //... and desktop shortcut (if requested)
        var pageWidget = gui.pageWidgetByObjectName("DynamicShortcutPage")
        if (pageWidget !== null && pageWidget.shortcutCheckBox.checked)
            component.addOperation("CreateShortcut",
                                   "@TargetDir@/bin/@ProductName@.exe",
                                   "@DesktopDir@/@Name@.lnk")
    }
}
