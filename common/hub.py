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
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from sitekit import lang_link, sticky_lang   # noqa: E402

ROOT = Path(__file__).resolve().parent.parent

SITES = {
    'nl': [
        ('../6x6_is_strongly_solved/nl/', '6×6 Is Strongly Solved',
         'Dammen op het 6×6-bord is sterk opgelost: elke stelling tot en met 12 stukken, met de exacte afstand tot het '
         'einde. De beginstand is remise; de openingsboom tot 4 ply, de langste winst per aantal stukken en de '
         'geschiedenis van de oplossing.'),
        ('../strategie_der_36_velden/', 'Strategie der 36 velden',
         'Herm Jan Brascamps reeks over het 6×6-bord (2016), hoofdstuk voor hoofdstuk nagerekend met exacte '
         'databases.'),
        ('../drie_tegen_een_is_gemeen/', 'Drie tegen één is gemeen',
         'Het boek van Gerrit L. de Bruijn en Aad N.J. Thoen (1988) over twee en drie dammen tegen één op borden van elke '
         'afmeting, nagerekend en uitgebreid: de winst op 6×5, de families, de vangstellingen en 10×10 met één veld weg.'),
        ('../checkers/', 'Checkers: 3 dammen tegen 2',
         'Korte dammen op borden van elke afmeting: blokkades, de vesting in de dubbele hoek en de oppositie.'),
    ],
    'en': [
        ('6x6_is_strongly_solved/', '6×6 Is Strongly Solved',
         'Draughts on the 6×6 board is strongly solved: every position up to 12 pieces, with the exact distance to the '
         'end. The start position is a draw; the opening tree to 4 plies, the longest win per number of pieces and the '
         'history of the solution.'),
        ('strategie_der_36_velden/', 'Strategie der 36 velden',
         'Herm Jan Brascamp\'s series on the 6×6 board (2016), checked chapter by chapter with exact databases.'),
        ('drie_tegen_een_is_gemeen/', 'Drie tegen één is gemeen',
         'The book by Gerrit L. de Bruijn and Aad N.J. Thoen (1988) on two and three kings against one on boards of any '
         'size, checked and extended: the win on 6×5, the families, the trapping positions and 10×10 with one square '
         'removed.'),
        ('checkers/', 'Checkers: 3 kings against 2',
         'Short kings on boards of any size: blockades, the double-corner fortress and opposition.'),
    ],
}

TEXT = {
    'nl': ('Eindspeldatabases voor het damspel',
           'Exacte eindspeldatabases voor het internationale damspel en voor checkers, op borden van elke afmeting: elke '
           'stelling is uitgerekend tot het einde van de partij, met de afstand in ply. Drie sites spelen klassieke '
           'analyses na en een vierde verzamelt de oplossing van 6×6, met klikbare zetten en bij elke stelling de waarde en alle zetten.',
           'De programma\'s, databases en rapporten', '../index.html', 'English'),
    'en': ('Endgame databases for draughts',
           'Exact endgame databases for international draughts and for checkers, on boards of any size: every position is '
           'computed to the end of the game, with the distance in plies. Three sites replay classic analyses and a fourth collects the solution of 6×6, with '
           'clickable moves and for every position the value and all moves.',
           'The programs, databases and reports', 'nl/index.html', 'Nederlands'),
}

CREDIT = {'nl': 'De vorm van de analyses is ontleend aan die van Wieger Wesselink op <a href="https://10x10.org">10x10.org</a>.',
          'en': 'The form of the analyses is taken from those by Wieger Wesselink on <a href="https://10x10.org">10x10.org</a>.'}

OLD = ['index.html', 'opening.html', 'voorbeeld-7-22.html'] + [f'{p}.html' for p in (
    'h1', 'h2', 'h3', 'h3-3', 'h4', 'h5a', 'h5b', 'h5c', 'h5d', 'h6a', 'h6b', 'h7', 'h6a-opl', 'h6-opl1', 'h6-opl2',
    'h6-opl3', 'h7-2', 'h7-3', 'h7-4', 'h7-5', 'h7-6', 'h7-7', 'h8', 'h9')]

PAGE = '''<!DOCTYPE html>
<html lang="{lang}">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
{sticky}
<title>egdb-draughts · {h1}</title>
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
.langs { float: right; display: flex; gap: 6px; }
.flag { display: inline-flex; width: 26px; height: 17px; border-radius: 3px; overflow: hidden; opacity: .45; outline: 1px solid var(--rule); }
.flag svg { width: 100%; height: 100%; display: block; }
a.flag:hover, a.flag:focus-visible { opacity: 1; }
.flag.cur { opacity: 1; outline: 2px solid var(--muted); outline-offset: 1px; }
</style>
</head>
<body>
<main>
{langs}
<h1>{h1}</h1>
<p>{intro}</p>
<div class="hub">
{cards}
</div>
<p class="src">{src}: <a href="https://github.com/rhalbersma/egdb-draughts">github.com/rhalbersma/egdb-draughts</a>. {credit}</p>
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
    for lang in ('nl', 'en'):
        h1, intro, src, alt, altname = TEXT[lang]
        cards = '\n'.join(f'<a href="{u}"><strong>{t}</strong><span>{d}</span></a>' for u, t, d in SITES[lang])
        page = PAGE
        for k, v in (('{cards}', cards), ('{lang}', lang), ('{h1}', h1), ('{intro}', intro), ('{src}', src),
                     ('{langs}', lang_link(lang, alt)), ('{credit}', CREDIT[lang]), ('{sticky}', sticky_lang('hub', lang, alt))):
            page = page.replace(k, v)
        d = out if lang == 'en' else out / 'nl'          # English by default, Dutch in nl/
        d.mkdir(parents=True, exist_ok=True)
        (d / 'index.html').write_text(page)
    for f in OLD[1:]:
        target = ('6x6_is_strongly_solved/nl/international.html' if f == 'opening.html' else
                  'strategie_der_36_velden/' + ('h7-7.html#d7.22' if f == 'voorbeeld-7-22.html' else f))
        (out / f).write_text(REDIRECT.format(url=target))
    (out / '.nojekyll').write_text('')


if __name__ == '__main__':
    main()
