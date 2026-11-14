#!/usr/bin/env python3
#          Copyright Rein Halbersma 2026.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE_1_0.txt or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

"""The start page of the companion sites (_site/index.html) and redirects from the old addresses of the
Strategie der 36 velden pages, which used to live at the top level.

usage: hub.py [--out _site]
"""

import argparse
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

SITES = [
    ('strategie_der_36_velden/', 'Strategie der 36 velden',
     'Herm Jan Brascamps reeks over het 6×6-bord (2016), hoofdstuk voor hoofdstuk nagerekend met exacte databases tot '
     '10 stukken; plus de oplossing van de beginstand: remise.'),
    ('drie_tegen_een_is_gemeen/', 'Drie tegen een is gemeen',
     'Twee en drie dammen tegen één op borden van elke afmeting: de winst op 5×6, de families, de vangstellingen en '
     '10×10 met één veld weg.'),
    ('checkers/', 'Checkers: 3 dammen tegen 2',
     'Korte dammen op borden van elke afmeting: blokkades, de vesting in de dubbele hoek en de oppositie.'),
]

OLD = ['index.html', 'opening.html', 'voorbeeld-7-22.html'] + [f'{p}.html' for p in (
    'h1', 'h2', 'h3', 'h3-3', 'h4', 'h5a', 'h5b', 'h5c', 'h5d', 'h6a', 'h6b', 'h7', 'h6a-opl', 'h6-opl1', 'h6-opl2',
    'h6-opl3', 'h7-2', 'h7-3', 'h7-4', 'h7-5', 'h7-6', 'h7-7', 'h8', 'h9')]

PAGE = '''<!DOCTYPE html>
<html lang="nl">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>egdb-draughts · eindspeldatabases</title>
<style>
:root { --bg: #fbfaf7; --fg: #1d1d1b; --muted: #6b6a66; --rule: #e3e0d8; --panel: #f3f1ec; --link: #1f5f8b; }
@media (prefers-color-scheme: dark) {
  :root:not([data-theme="light"]) { --bg: #161514; --fg: #e9e6df; --muted: #a29e95; --rule: #34312d; --panel: #201e1c; --link: #7fb6dd; }
}
:root[data-theme="dark"] { --bg: #161514; --fg: #e9e6df; --muted: #a29e95; --rule: #34312d; --panel: #201e1c; --link: #7fb6dd; }
* { box-sizing: border-box; }
body { margin: 0; background: var(--bg); color: var(--fg); font: 17px/1.6 Charter, "Bitstream Charter", "Sitka Text", Cambria, Georgia, serif; }
main { max-width: 860px; margin: 0 auto; padding: 40px 16px 64px; }
h1 { font-size: 32px; line-height: 1.2; margin: 0 0 12px; }
p { margin: 0 0 14px; }
a { color: var(--link); }
.hub { display: grid; grid-template-columns: repeat(auto-fit, minmax(240px, 1fr)); gap: 16px; margin: 28px 0; }
.hub a { display: block; border: 1px solid var(--rule); border-radius: 8px; padding: 16px 18px; background: var(--panel); color: var(--fg); text-decoration: none; }
.hub a:hover { border-color: var(--muted); }
.hub strong { display: block; font-size: 20px; margin-bottom: 6px; color: var(--link); }
.hub span { font-size: 15px; color: var(--muted); }
.src { font: 14px system-ui, sans-serif; color: var(--muted); }
</style>
</head>
<body>
<main>
<h1>Eindspeldatabases voor het damspel</h1>
<p>Exacte eindspeldatabases voor het internationale damspel en voor checkers, op borden van elke afmeting: elke
stelling is uitgerekend tot het einde van de partij, met de afstand in ply. Drie sites spelen klassieke analyses na,
met klikbare zetten en bij elke stelling de waarde en alle zetten.</p>
<div class="hub">
{cards}
</div>
<p class="src">De programma's, databases en rapporten: <a href="https://github.com/rhalbersma/egdb-draughts">github.com/rhalbersma/egdb-draughts</a>.</p>
</main>
</body>
</html>
'''

REDIRECT = ('<!DOCTYPE html><meta charset="utf-8"><meta http-equiv="refresh" content="0; url={url}">'
            '<link rel="canonical" href="{url}"><title>Strategie der 36 velden</title><a href="{url}">{url}</a>\n')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--out', default=str(ROOT / '_site'))
    args = ap.parse_args()
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    cards = '\n'.join(f'<a href="{u}"><strong>{t}</strong><span>{d}</span></a>' for u, t, d in SITES)
    (out / 'index.html').write_text(PAGE.replace('{cards}', cards))
    for f in OLD[1:]:
        target = 'strategie_der_36_velden/' + ('h7-7.html#d7.22' if f == 'voorbeeld-7-22.html' else f)
        (out / f).write_text(REDIRECT.format(url=target))
    (out / '.nojekyll').write_text('')


if __name__ == '__main__':
    main()
