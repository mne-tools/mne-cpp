/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 */

import React, { useState } from 'react';
import clsx from 'clsx';
import Link from '@docusaurus/Link';
import { useBaseUrlUtils } from '@docusaurus/useBaseUrl';
import { useLocation } from '@docusaurus/router';
import { topicById, type Topic } from '../knowledge';
import styles from './styles.module.css';

type Context = 'manual' | 'api' | 'example';

interface Props {
    /** One topic id or several (doc/api_registry.json `topics`). */
    topic?: string;
    topics?: string[];
    /** Kind of page the panel sits on; that column marks the current page. */
    context?: Context;
    /** Class or example name of the current page. */
    current?: string;
}

const ICON_PATHS: Record<Context, React.ReactNode> = {
    manual: <path d="M4 5.5A2.5 2.5 0 0 1 6.5 3H20v15H6.5A2.5 2.5 0 0 0 4 20.5m0-15v15m0 0A2.5 2.5 0 0 0 6.5 23H20v-5" />,
    api: <path d="M8 4c-2 0-3 1-3 3v2.5c0 1.2-.8 2-2 2.5 1.2.5 2 1.3 2 2.5V17c0 2 1 3 3 3m8-16c2 0 3 1 3 3v2.5c0 1.2.8 2 2 2.5-1.2.5-2 1.3-2 2.5V17c0 2-1 3-3 3" />,
    example: <path d="M7 4.5v15l12-7.5L7 4.5Z" />,
};

function Icon({ kind }: { kind: Context }) {
    return (
        <svg className={styles.icon} viewBox="0 0 24 24" aria-hidden="true">
            {ICON_PATHS[kind]}
        </svg>
    );
}

function Panel({ topic, context, current }: { topic: Topic; context?: Context; current?: string }) {
    const { pathname } = useLocation();
    const { withBaseUrl } = useBaseUrlUtils();
    const here = (url: string) => pathname === withBaseUrl(url);
    return (
        <div className={styles.grid}>
            <section aria-label="Understand the method" className={clsx(styles.column, context === 'manual' && styles.columnHere)}>
                <div className={styles.columnTitle}><Icon kind="manual" />Understand the method</div>
                <ul className={styles.list}>
                    {topic.manual.map((m) => (
                        <li key={m.url}>
                            <Link to={m.url} className={clsx(styles.link, here(m.url.split('#')[0]) && styles.linkHere)}>
                                {m.label}
                            </Link>
                        </li>
                    ))}
                </ul>
                {topic.tools.length > 0 && (
                    <>
                        <div className={styles.subTitle}>Command-line tools</div>
                        <ul className={clsx(styles.list, styles.inline)}>
                            {topic.tools.map((t) => (
                                <li key={t.url}>
                                    <Link to={t.url} className={styles.code}>{t.label}</Link>
                                </li>
                            ))}
                        </ul>
                    </>
                )}
            </section>
            <section aria-label="Use the API" className={clsx(styles.column, context === 'api' && styles.columnHere)}>
                <div className={styles.columnTitle}><Icon kind="api" />Use the API</div>
                <ul className={styles.list}>
                    {topic.api.map((a) => (
                        <li key={a.name}>
                            <Link to={a.url} className={clsx(styles.code, a.name === current && context === 'api' && styles.linkHere)}>
                                {a.name}
                            </Link>
                            {a.methods.length > 0 && (
                                <span className={styles.methods}>
                                    {a.methods.map((m) => (
                                        <Link key={m.url} to={m.url} className={styles.method}>::{m.name}()</Link>
                                    ))}
                                </span>
                            )}
                        </li>
                    ))}
                </ul>
            </section>
            <section aria-label="Run an example" className={clsx(styles.column, context === 'example' && styles.columnHere)}>
                <div className={styles.columnTitle}><Icon kind="example" />Run an example</div>
                <ul className={styles.list}>
                    {topic.examples.map((e) => (
                        <li key={e.name}>
                            <Link to={e.url} className={clsx(e.name === current && context === 'example' && styles.linkHere)}
                                title={`${e.name} — ${e.brief}`}>
                                {e.title}
                            </Link>
                            {e.start && <span className={styles.start}>Start here</span>}
                        </li>
                    ))}
                </ul>
            </section>
        </div>
    );
}

export default function Connections({ topic, topics, context, current }: Props): React.JSX.Element | null {
    const ids = topics ?? (topic ? [topic] : []);
    const resolved = ids.map(topicById).filter((t): t is Topic => Boolean(t));
    const [active, setActive] = useState(0);
    if (resolved.length === 0) {
        return null;
    }
    const shown = resolved[Math.min(active, resolved.length - 1)];
    return (
        <aside className={styles.connections} aria-label={`Related resources: ${shown.title}`}>
            <header className={styles.header}>
                <div className={styles.eyebrow}>Method · API · Example</div>
                {resolved.length > 1 ? (
                    <div className={styles.tabs} role="tablist" aria-label="Topics">
                        {resolved.map((t, i) => (
                            <button
                                key={t.id}
                                type="button"
                                role="tab"
                                aria-selected={i === active}
                                className={clsx(styles.tab, i === active && styles.tabActive)}
                                onClick={() => setActive(i)}>
                                {t.title}
                            </button>
                        ))}
                    </div>
                ) : (
                    <div className={styles.title}>{shown.title}</div>
                )}
                <p className={styles.summary}>{shown.summary}</p>
            </header>
            <Panel topic={shown} context={context} current={current} />
        </aside>
    );
}
