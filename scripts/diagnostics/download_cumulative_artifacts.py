"""Download only the two pinned public ComboShip Actions artifacts.

The Actions token is sent only to api.github.com. Redirected storage requests
receive no authorization header. Receipts contain public metadata and hashes.
"""
import concurrent.futures
import hashlib
import json
import os
from pathlib import Path
import urllib.error
import urllib.parse
import urllib.request

REPO = 'marsh6487/ComboShip'
PINS = [
    {'name': 'candidate', 'id': 12716106505, 'run': 40459338133,
     'head': '0cd35e4d25f853dc68995dd88e8b1c177840e301',
     'bytes': 402867885,
     'sha256': '39e676d57390b1fa8b07ae8922dfad828a81576a2923e66e0985b07ce5199116'},
    {'name': 'baseline', 'id': 12656287832, 'run': 40328880328,
     'head': 'fa81f7926038f2aa85c7f3e73c96fec043168b66',
     'bytes': 401424147,
     'sha256': 'eec89cc0ef6dde94005f58d1e9e5a977469857a7666dac604dbbf02fea915c2e'},
]

class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, *args, **kwargs):
        return None

def request(path):
    return urllib.request.Request(
        'https://api.github.com/repos/' + REPO + path,
        headers={'Authorization': 'Bearer ' + os.environ['GH_TOKEN'],
                 'Accept': 'application/vnd.github+json',
                 'X-GitHub-Api-Version': '2022-11-28'})

def download(pin):
    opener = urllib.request.build_opener(NoRedirect())
    with opener.open(request('/actions/artifacts/' + str(pin['id'])), timeout=60) as r:
        meta = json.load(r)
    assert meta['name'] == 'ComboShip-windows' and not meta['expired']
    assert meta['workflow_run']['id'] == pin['run']
    assert meta['workflow_run']['head_sha'] == pin['head']
    assert meta['size_in_bytes'] == pin['bytes']
    assert meta['digest'] == 'sha256:' + pin['sha256']
    try:
        incoming = opener.open(request('/actions/artifacts/' + str(pin['id']) + '/zip'), timeout=60)
    except urllib.error.HTTPError as e:
        if e.code not in (301, 302, 303, 307, 308):
            raise
        location = e.headers['Location']
        parsed = urllib.parse.urlsplit(location)
        if parsed.scheme != 'https' or parsed.username or parsed.password:
            raise ValueError('Invalid artifact redirect')
        incoming = urllib.request.urlopen(location, timeout=60)
    output = Path('downloaded') / (pin['name'] + '.zip')
    output.parent.mkdir(exist_ok=True)
    digest = hashlib.sha256()
    size = 0
    with incoming, output.open('wb') as stream:
        while block := incoming.read(1024 * 1024):
            stream.write(block)
            digest.update(block)
            size += len(block)
    assert size == pin['bytes']
    assert digest.hexdigest() == pin['sha256']
    receipt = {**pin, 'path': str(output), 'verified_sha256': digest.hexdigest()}
    print(json.dumps(receipt), flush=True)
    return receipt

if __name__ == '__main__':
    with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
        receipts = list(pool.map(download, PINS))
    Path('evidence').mkdir(exist_ok=True)
    Path('evidence/download-receipt.json').write_text(json.dumps(receipts, indent=2) + '\n')
