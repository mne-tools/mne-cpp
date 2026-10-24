/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 */

import React, { useEffect, useMemo, useState } from 'react';
import clsx from 'clsx';
import Link from '@docusaurus/Link';
import useBaseUrl from '@docusaurus/useBaseUrl';
import { useHistory, useLocation } from '@docusaurus/router';
import { examples, facets, libraries, topics, type Example } from '../knowledge';
import styles from './styles.module.css';

type Filter = 'task' | 'modality' | 'level' | 'library';
type Selection = Record<Filter, string>;

const EMPTY: Selection = { task: '', modality: '', level: '', library: '' };
const LEVEL_ORDER = Object.keys(facets.levels);

function matches(example: Example, selection: Selection, terms: string[]): boolean {
    if (selection.task && !example.tasks.includes(selection.task)) return false;
    if (selection.modality && !example.modalities.includes(selection.modality)) return false;
    if (selection.level && example.level !== selection.level) return false;
    if (selection.library && !example.libraries.includes(selection.library)) return false;
    if (terms.length === 0) return true;
    const topicTitles = example.topics.map((id) => topics.find((t) => t.id === id)?.title ?? '');
    const haystack = [example.title, example.name, example.brief, ...example.libraries.map((l) => libraries[l]), ...topicTitles]
        .join(' ')
        .toLowerCase();
    return terms.every((term) => haystack.includes(term));
}

function count(filter: Filter, value: string, selection: Selection, terms: string[]): number {
    const probe = { ...selection, [filter]: value };
    return examples.filter((e) => matches(e, probe, terms)).length;
}

function Chips({ filter, label, options, selection, terms, onSelect }: {
    filter: Filter;
    label: string;
    options: Record<string, string>;
    selection: Selection;
    terms: string[];
    onSelect: (filter: Filter, value: string) => void;
}) {
    return (
        <div className={styles.facet} role="group" aria-label={label}>
            <div className={styles.facetLabel}>{label}</div>
            <div className={styles.chips}>
                {Object.entries(options).map(([value, text]) => {
                    const active = selection[filter] === value;
                    const n = count(filter, value, selection, terms);
                    return (
                        <button
                            key={value}
                            type="button"
                            aria-pressed={active}
                            disabled={!active && n === 0}
                            className={clsx(styles.chip, active && styles.chipActive)}
                            onClick={() => onSelect(filter, active ? '' : value)}>
                            {text}
                            <span className={styles.chipCount}>{n}</span>
                        </button>
                    );
                })}
            </div>
        </div>
    );
}

function Card({ example }: { example: Example }) {
    const thumbnail = useBaseUrl(example.thumbnail || '/');
    return (
        <li className={styles.card}>
            <Link to={example.url} className={styles.cardLink}>
                <div className={clsx(styles.thumb, !example.thumbnail && styles.thumbEmpty)} aria-hidden="true">
                    {example.thumbnail ? (
                        <img src={thumbnail} alt="" loading="lazy" />
                    ) : (
                        <span className={styles.thumbLabel}>{libraries[example.library]}</span>
                    )}
                </div>
                <div className={styles.cardBody}>
                    <span className={styles.title}>{example.title}</span>
                    <p className={styles.brief}>{example.brief}.</p>
                </div>
            </Link>
            <div className={styles.cardFooter}>
                <code className={styles.program}>{example.name}</code>
                <span className={clsx('mc-tag', example.level === 'start' && 'mc-tag--teal')}>{facets.levels[example.level]}</span>
            </div>
        </li>
    );
}

export default function ExamplesExplorer(): React.JSX.Element {
    const location = useLocation();
    const history = useHistory();
    const [query, setQuery] = useState('');
    const [selection, setSelection] = useState<Selection>(EMPTY);

    // Facet links on example pages open the explorer pre-filtered (?task=…&modality=…).
    useEffect(() => {
        const params = new URLSearchParams(location.search);
        setSelection({
            task: params.get('task') ?? '',
            modality: params.get('modality') ?? '',
            level: params.get('level') ?? '',
            library: params.get('library') ?? '',
        });
        setQuery(params.get('q') ?? '');
    }, [location.search]);

    const update = (next: Selection, q: string) => {
        const params = new URLSearchParams();
        (Object.keys(next) as Filter[]).forEach((k) => next[k] && params.set(k, next[k]));
        if (q) params.set('q', q);
        const search = params.toString();
        history.replace({ ...location, search: search ? `?${search}` : '' });
    };

    const select = (filter: Filter, value: string) => {
        const next = { ...selection, [filter]: value };
        setSelection(next);
        update(next, query);
    };

    const terms = useMemo(() => query.toLowerCase().split(/\s+/).filter(Boolean), [query]);
    const results = useMemo(
        () =>
            examples
                .filter((e) => matches(e, selection, terms))
                .sort((a, b) => LEVEL_ORDER.indexOf(a.level) - LEVEL_ORDER.indexOf(b.level)
                    || Number(Boolean(b.thumbnail)) - Number(Boolean(a.thumbnail)) || a.title.localeCompare(b.title)),
        [selection, terms],
    );
    const usedLibraries = useMemo(
        () => Object.fromEntries(Object.entries(libraries).filter(([key]) => examples.some((e) => e.libraries.includes(key)))),
        [],
    );
    const filtered = query !== '' || Object.values(selection).some(Boolean);

    return (
        <div className={styles.explorer}>
            <div className={styles.searchRow}>
                <label className={styles.search}>
                    <svg viewBox="0 0 24 24" aria-hidden="true" className={styles.searchIcon}>
                        <circle cx="11" cy="11" r="6.5" />
                        <path d="m16 16 4.5 4.5" />
                    </svg>
                    <span className="sr-only">Search examples</span>
                    <input
                        type="search"
                        value={query}
                        placeholder="Search by name, method or class — e.g. dSPM, coherence, FIFF"
                        onChange={(e) => {
                            setQuery(e.target.value);
                            update(selection, e.target.value);
                        }}
                    />
                </label>
                <label className={styles.select}>
                    <span className={styles.facetLabel}>Library</span>
                    <select value={selection.library} onChange={(e) => select('library', e.target.value)}>
                        <option value="">All libraries</option>
                        {Object.entries(usedLibraries).map(([key, label]) => (
                            <option key={key} value={key}>
                                {label} ({count('library', key, selection, terms)})
                            </option>
                        ))}
                    </select>
                </label>
            </div>
            <Chips filter="task" label="What do you want to do?" options={facets.tasks} selection={selection} terms={terms} onSelect={select} />
            <div className={styles.facetRow}>
                <Chips filter="modality" label="Recordings" options={facets.modalities} selection={selection} terms={terms} onSelect={select} />
                <Chips filter="level" label="Level" options={facets.levels} selection={selection} terms={terms} onSelect={select} />
            </div>

            <div className={styles.status} aria-live="polite">
                <span>
                    {results.length === examples.length ? `All ${examples.length} examples` : `${results.length} of ${examples.length} examples`}
                </span>
                {filtered && (
                    <button type="button" className={styles.reset} onClick={() => { setQuery(''); setSelection(EMPTY); update(EMPTY, ''); }}>
                        Clear filters
                    </button>
                )}
            </div>

            {results.length === 0 ? (
                <div className={styles.empty}>
                    <p>No example matches these filters.</p>
                    <p>
                        Try fewer filters, or search the{' '}
                        <Link to="/docs/api/">API reference</Link> for the class you are interested in.
                    </p>
                </div>
            ) : (
                <ul className={styles.cards}>
                    {results.map((e) => <Card key={e.name} example={e} />)}
                </ul>
            )}
        </div>
    );
}
