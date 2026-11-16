"""Teste Windows opcional (Python 3); o programa em C nao depende de Python.

Usa um console isolado e oculto; le caracteres Unicode do buffer REAL do console,
alem de conferir os bytes redirecionados. Nao acessa os arquivos de estoque.
"""
import ctypes
from ctypes import wintypes as w
import json
import os
from pathlib import Path
import subprocess
import sys

WORDS = ['Op\u00e7\u00e3o', 'C\u00f3digo', 'Pre\u00e7o', 'Atualiza\u00e7\u00e3o']


def console_test(exe, page):
    import msvcrt

    k = ctypes.WinDLL('kernel32', use_last_error=True)

    class COORD(ctypes.Structure):
        _fields_ = [('X', w.SHORT), ('Y', w.SHORT)]

    k.CreateFileW.argtypes = [w.LPCWSTR, w.DWORD, w.DWORD, w.LPVOID,
                              w.DWORD, w.DWORD, w.HANDLE]
    k.CreateFileW.restype = w.HANDLE
    k.SetConsoleOutputCP.argtypes = [w.UINT]
    k.SetConsoleOutputCP.restype = w.BOOL
    k.GetConsoleOutputCP.restype = w.UINT
    k.ReadConsoleOutputCharacterW.argtypes = [w.HANDLE, w.LPWSTR, w.DWORD,
                                             COORD, ctypes.POINTER(w.DWORD)]
    k.ReadConsoleOutputCharacterW.restype = w.BOOL
    assert k.SetConsoleOutputCP(page), ctypes.get_last_error()
    handle = k.CreateFileW('CONOUT$', 0xC0000000, 3, None, 3, 0, None)
    assert handle != ctypes.c_void_p(-1).value, ctypes.get_last_error()
    # open_osfhandle transfere a propriedade do handle ao objeto de arquivo.
    with os.fdopen(msvcrt.open_osfhandle(handle, os.O_WRONLY), 'wb', buffering=0) as output:
        result = subprocess.run([exe, '--testar-acentos'], stdin=subprocess.DEVNULL,
                                stdout=output, stderr=subprocess.PIPE, timeout=10)
        assert result.returncode == 0, result.stderr
        buffer = ctypes.create_unicode_buffer(2048)
        count = w.DWORD()
        assert k.ReadConsoleOutputCharacterW(handle, buffer, 2048, COORD(0, 0),
                                             ctypes.byref(count)), ctypes.get_last_error()
        text = buffer[:count.value]
    assert k.GetConsoleOutputCP() == 65001
    assert 'antes: %d; durante: 65001' % page in text, ascii(text)
    for word in WORDS:
        assert word in text, (ascii(word), ascii(text))
    return {'pagina_inicial': page, 'pagina_final': 65001,
            'buffer_unicode': 'OK', 'palavras': WORDS}


def main():
    if sys.platform != 'win32':
        raise SystemExit('Este teste de console requer Windows.')
    exe = os.environ.get('ESTOQUE_TESTE_EXE',
                         str(Path(__file__).resolve().parents[1] / 'estoque.exe'))
    if len(sys.argv) == 3 and sys.argv[1] == '--console':
        print(json.dumps(console_test(exe, int(sys.argv[2])), ensure_ascii=True))
        return
    result = subprocess.run([exe, '--testar-acentos'], capture_output=True, timeout=10)
    assert result.returncode == 0, result.stderr
    text = result.stdout.decode('utf-8', errors='strict')
    for word in WORDS:
        assert (word + '\r\n').encode('utf-8') in result.stdout
    print('PASSOU: bytes redirecionados UTF-8, sem conversao dupla.')
    for page in (437, 65001):
        startup = subprocess.STARTUPINFO()
        startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
        startup.wShowWindow = 0  # SW_HIDE: nenhum console visivel.
        result = subprocess.run([sys.executable, str(Path(__file__).resolve()),
                                 '--console', str(page)], capture_output=True,
                                startupinfo=startup,
                                creationflags=subprocess.CREATE_NEW_CONSOLE, timeout=20)
        assert result.returncode == 0, result.stderr.decode(errors='replace')
        data = json.loads(result.stdout)
        print('PASSOU: console real %d -> %d; quatro palavras corretas no buffer Unicode.'
              % (data['pagina_inicial'], data['pagina_final']))


if __name__ == '__main__':
    main()
