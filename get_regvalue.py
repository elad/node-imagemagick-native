import subprocess

def get_regvalue(regkey, regvalue):
    cmd = ['reg.exe', 'query', regkey, '/v', regvalue]
    
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    for line in p.communicate()[0].splitlines():
        if regvalue in line:
            data = line.split('    ')[3] + "\\"
    return data

print get_regvalue('HKEY_LOCAL_MACHINE\\SOFTWARE\\ImageMagick\\Current', "LibPath")