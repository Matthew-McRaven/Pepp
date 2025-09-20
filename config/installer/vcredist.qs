// packaging/vcruntime.qs
function Component() {}

Component.prototype.createOperations = function() {
    component.createOperations();

    var base = installer.value("TargetDir") + "/vcredist/";
    var exe = base + "VC_redist.x64.exe";

    console.log("Installing Visual C++ Redistributable from " + exe);
    // Run quietly and elevated; suppress reboot.
    component.addElevatedOperation("Execute", exe, "/install", "/quiet", "/norestart");
};
