import argparse
import os
import json

def migrate_figure(type: str, path: str):
    with open(path, 'r') as f: figure = json.load(f)
    out = {
        "version": 2,
        "type": type,
        "name": figure.get("name"),
        "arch": figure.get("arch"),
    }
    if "abstraction" in figure: out["abstraction"] = figure["abstraction"]
    elif "ch12" in path: out["abstraction"] = "MC2"
    elif "os" in path: out["abstraction"] = "OS4"
    elif "cs4e" in path and "ch04" in path: out["abstraction"] = "ISA3"
    elif "cs4e" in path and "ch05" in path: out["abstraction"] = "ASMB5"
    elif "cs4e" in path and "ch06" in path: out["abstraction"] = "ASMB5"
    elif type == "problem" and "cs4e" in path and "ch08" in path: out["abstraction"] = "OS4"
    if "default_os" in figure: out["default_os"] = figure["default_os"]
    if "description" in figure: out["description"] = figure["description"]
    if "ios" in figure: out["tests"] = figure["ios"]

    def make_item(format, name=None, hidden=False, copy=None):
        ret = dict()
        if name is not None: ret["name"]=name
        ret["format"] = format
        if hidden: ret["isHidden"] = True
        if copy is not None: ret["copy"] = copy
        if "default_element" in figure and figure["default_element"] == format: ret["isDefault"] = True
        return ret
    def add_from_file(item, path): item["from"] = {"file":path}
    def add_from_element(item, element): item["from"] = {"element": element}
    assert "abstraction" in out, f"Need an abstraction for {path}"
    if out["abstraction"] == "ISA3":
        out["items"]=[]
        # Add existing figures
        for format in figure.get("items", []):
            el = make_item(format, name=format)
            if format == "ISA3": el["copy"] ="object"
            add_from_file(el, figure["items"][format])
            out["items"].append(el)
        # Insert pepb/peph
        if "pep" in figure["items"]:
          pepb, peph = make_item("pepb", name="pepb"), make_item("peph", name="peph")
          add_from_element(pepb, "pep"), add_from_element(peph, "pep")
          out["items"].extend([pepb, peph])
    elif out["abstraction"] == "ASMB3" or out["abstraction"] == "ASMB5" or out["abstraction"] == "OS4":
        out["items"]=[]
        # Add existing figures
        for format in figure.get("items", []):
            el = make_item(format, name=format)
            if format == "pep": el["copy"] ="assembly"
            add_from_file(el, figure["items"][format])
            out["items"].append(el)
        # Insert pepl/pepo
        if "pep" in figure["items"] and out["abstraction"] != "OS4":
          pepl, pepo = make_item("pepl", name="pepl"), make_item("pepo", name="pepo")
          add_from_element(pepl, "pep"), add_from_element(pepo, "pep")
          out["items"].extend([pepl, pepo])
    elif out["abstraction"] == "MC2":
        out["items"]=[]
        for format in figure.get("items", []):
            el = make_item(format, name=format)
            if format == "MC2": el["copy"] ="microcode"
            add_from_file(el, figure["items"][format])
            out["items"].append(el)
    else:
        assert False, "Unknown abstraction: " + out["abstraction"]
    manifest = os.path.join(os.path.dirname(path),"manifest.json")
    with open(manifest, 'w') as f: json.dump(out, f, indent=2)
    os.remove(path)

def main():
    parser = argparse.ArgumentParser(description="Migrate figure manifest versions")
    parser.add_argument("path", type=str, help="Path to the book manifest file.")
    args = parser.parse_args()
    # Recursively enumerate all figures/problems/macros in the directory
    for root, subFolders, files in os.walk(args.path):
        for file in files:
            try:
                match file:
                    case "figure.json":
                        migrate_figure("figure", os.path.join(root, file))
                    case "problem.json":
                            migrate_figure("problem", os.path.join(root, file))
                    case _:
                        continue
            except FileNotFoundError:
                print(f"Error: File '{filename}' not found.")
            except json.JSONDecodeError:
                print(f"Error: Invalid JSON format in '{filename}'.")

if __name__ == "__main__":
    main()
