/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 */

import React from 'react';
import clsx from 'clsx';
import useBaseUrl from '@docusaurus/useBaseUrl';
import styles from './styles.module.css';

/** Figures an example wrote with DISPLIB::FigureExporter; one per row on narrow screens, two otherwise. */
export function ExampleFigures({ children }: { children: React.ReactNode }): React.JSX.Element {
    const single = React.Children.count(children) === 1;
    return <div className={clsx(styles.figures, single && styles.single)}>{children}</div>;
}

/** One captioned figure; a click opens it at full resolution. */
export function ExampleFigure({ src, caption }: { src: string; caption: string }): React.JSX.Element {
    const url = useBaseUrl(src);
    return (
        <figure className={styles.figure}>
            <a href={url} target="_blank" rel="noopener noreferrer" className={styles.frame} aria-label={`Open figure: ${caption}`}>
                <img src={url} alt={caption} loading="lazy" />
            </a>
            {caption && <figcaption>{caption}</figcaption>}
        </figure>
    );
}
