import subprocess, re

class Term:
    def __init__(self, path):
        self.path = path
    def run(self, *args):
        out = subprocess.run([self.path, *args], capture_output=True)
        return out
    def mit(self):
        out = self.run("mit")
        as_str = out.stdout.decode("utf-8")
        m = re.search(r"Throughput was: (\d\.\d+)e\+(\d+)", as_str, re.RegexFlag.M)
        try: return float(m.group(1)) * 10**int(m.group(2))
        # If match fails, return 0
        except IndexError: return 0
        except AttributeError: return 0
    def get_fig(self, ch, fig):
        out = self.run("get", "--ch", ch, "--fig", fig)
        out.check_returncode()
        return out.stdout.decode("utf-8")