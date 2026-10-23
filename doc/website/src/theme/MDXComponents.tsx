/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 */

// Components every MDX page may use without an import, including the generated API and example pages.
import MDXComponents from '@theme-original/MDXComponents';
import Connections from '@site/src/components/Connections';
import ExampleFacts from '@site/src/components/ExampleFacts';
import ExamplesExplorer from '@site/src/components/ExamplesExplorer';

export default {
    ...MDXComponents,
    Connections,
    ExampleFacts,
    ExamplesExplorer,
};
