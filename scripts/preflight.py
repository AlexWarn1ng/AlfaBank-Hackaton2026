"""Read-only local preflight: run `python scripts/preflight.py` from repository root."""
from pathlib import Path
import os
import sys
root=Path(__file__).resolve().parents[1]
problems=[]
for rel in ('compose.yaml','frontend/Dockerfile','company-service/Dockerfile',
            'news-analyzer/NewsAnalyzer/Dockerfile','db/init/hackaton.sql',
            'THIRD_PARTY_NOTICES.md'):
    if not (root/rel).is_file(): problems.append(f'Missing: {rel}')
env_file=root/'.env'
if not env_file.exists():
    problems.append('Missing .env: copy .env.example to .env and set credentials')
else:
    values={}
    for line in env_file.read_text(encoding='utf8').splitlines():
        line=line.strip()
        if line and not line.startswith('#') and '=' in line:
            k,v=line.split('=',1); values[k.strip()]=v.strip()
    for key in ('AI_API_KEY','COMPANY_AI_API_KEY','POSTGRES_PASSWORD','FLASK_SECRET_KEY'):
        value=values.get(key,'')
        if not value or value.startswith('CHANGE_ME'):
            problems.append(f'{key}: supply a real value in .env')
print('Alfa Insight preflight:')
for item in problems:print('  [!] '+item)
if not problems: print('  [OK] Basic files and configuration found (not an end-to-end test)')
sys.exit(bool(problems))
