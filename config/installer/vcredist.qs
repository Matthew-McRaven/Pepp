// packaging/vcruntime.qs
function Component() {}

Component.prototype.createOperations = function() {
    component.createOperations();

    var base = installer.value("TargetDir") + "/vcredist/";
    var exe = base + "VC_redist.x64.exe";

    // Treat these as success:
    // 0    : success
    // 3010 : success, reboot required
    // 5100 : vcredist-specific “newer/blocked”
    try {
      component.addElevatedOperation(
          "Execute",
          "{0,3010,5100}",
          exe, "/install", "/quiet", "/norestart"
      );
    } catch (e) {
      console.log(e)
    }
};
