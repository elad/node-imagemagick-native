import _winreg

def get_regvalue(regkey, regvalue):

    explorer = _winreg.OpenKey(
        _winreg.HKEY_LOCAL_MACHINE,
        regkey
    )
    
    value, type = _winreg.QueryValueEx(explorer, regvalue)

    return value

print get_regvalue('SOFTWARE\\ImageMagick\\Current', 'LibPath')