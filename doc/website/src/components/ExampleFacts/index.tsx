/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 */

import React from 'react';
import Link from '@docusaurus/Link';
import { exampleByName, facets } from '../knowledge';

/** Level, tasks and modalities of an example, each linking to the filtered explorer. */
export default function ExampleFacts({ name }: { name: string }): React.JSX.Element | null {
    const example = exampleByName(name);
    if (!example) {
        return null;
    }
    const tags: { key: string; label: string; to: string; tone?: string }[] = [
        { key: `level-${example.level}`, label: facets.levels[example.level], to: `/docs/examples/?level=${example.level}`, tone: 'mc-tag--teal' },
        ...example.tasks.map((t) => ({ key: `task-${t}`, label: facets.tasks[t], to: `/docs/examples/?task=${t}`, tone: 'mc-tag--accent' })),
        ...example.modalities.map((m) => ({ key: `mod-${m}`, label: facets.modalities[m], to: `/docs/examples/?modality=${m}` })),
    ];
    return (
        <div style={{ display: 'flex', flexWrap: 'wrap', gap: '0.4rem', margin: '-0.25rem 0 1.1rem' }}>
            {tags.map((tag) => (
                <Link key={tag.key} to={tag.to} className={`mc-tag ${tag.tone ?? ''}`} style={{ textDecoration: 'none' }}>
                    {tag.label}
                </Link>
            ))}
        </div>
    );
}
