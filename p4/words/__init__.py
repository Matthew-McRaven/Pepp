# Given an object/module, return the names that look like FORTH implementation functions
def extract(object): return [item:=getattr(object, key) for key in dir(object) if "FORTH" in dir(getattr(object, key))]
