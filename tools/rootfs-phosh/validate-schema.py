#!/usr/bin/env python3
"""Validate every override key against the actual installed schema XML."""
import configparser
from pathlib import Path
import xml.etree.ElementTree as ET

HERE = Path(__file__).resolve().parent
ROOTFS = Path('/root/rootfs-phosh-v1')
schemas = {}
for source in (ROOTFS / 'usr/share/glib-2.0/schemas').glob('*.gschema.xml'):
    for schema in ET.parse(source).getroot().findall('schema'):
        schemas[schema.attrib['id']] = {key.attrib['name']: key for key in schema.findall('key')}
override = configparser.ConfigParser(interpolation=None)
override.read(HERE / 'overlay/usr/share/glib-2.0/schemas/99-cupid-phosh-bringup.gschema.override')
records = []
for schema in override.sections():
    for key, value in override[schema].items():
        if key not in schemas.get(schema, {}):
            raise SystemExit(f'Schema/key missing in installed packages: {schema} {key}')
        definition = schemas[schema][key]
        description = ''.join(definition.itertext()).strip()
        if 'deprecated and ignored' in description.lower():
            raise SystemExit(f'Refusing ignored schema key: {schema} {key}')
        records.append(f'{schema}\t{key}\t{value}\ttype={definition.get("type", definition.get("enum", ""))}')
(HERE / 'schema-verified.txt').write_text('\n'.join(records) + '\n')
print(f'Installed schema keys validated: {len(records)}')
