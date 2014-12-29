import _winreg

def get_regvalue(regkey, regvalue):

    explorer = _winreg.OpenKey(
        _winreg.HKEY_LOCAL_MACHINE,
        regkey,
        0,
        _winreg.KEY_READ | _winreg.KEY_WOW64_64KEY
    )
    
    value, type = _winreg.QueryValueEx(explorer, regvalue)

    return value

print get_regvalue('SOFTWARE\\ImageMagick\\Current', 'LibPath')
