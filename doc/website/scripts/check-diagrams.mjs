// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 MNE-CPP Authors
//   Christoph Dinh <christoph.dinh@mne-cpp.org>
//
// Parses every ```mermaid block under docs/ with the site's own Mermaid. Diagrams are rendered in the
// browser, so the Docusaurus build accepts broken ones and the page shows a parse error instead.
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { JSDOM } from 'jsdom';

const dom = new JSDOM('<!doctype html><html><body></body></html>');
for (const name of ['window', 'document', 'navigator', 'Element', 'HTMLElement', 'Node', 'DOMParser', 'SVGElement']) {
  const value = name === 'window' ? dom.window : dom.window[name];
  Object.defineProperty(globalThis, name, { value, configurable: true, writable: true });
}
const { default: mermaid } = await import('mermaid');
mermaid.initialize({ startOnLoad: false });

const docs = path.join(path.dirname(fileURLToPath(import.meta.url)), '..', 'docs');
const pages = fs.readdirSync(docs, { recursive: true }).filter(name => /\.mdx?$/.test(name));
let diagrams = 0;
const failures = [];
for (const page of pages) {
  const text = fs.readFileSync(path.join(docs, page), 'utf8');
  for (const match of text.matchAll(/```mermaid\r?\n([\s\S]*?)```/g)) {
    diagrams++;
    try {
      await mermaid.parse(match[1]);
    } catch (error) {
      failures.push(`docs/${page}: ${String(error.message).split('\n').pop()}`);
    }
  }
}
for (const failure of failures) console.error(`FAIL: ${failure}`);
console.log(`${diagrams} Mermaid diagrams in ${pages.length} pages, ${failures.length} fail to parse`);
process.exit(failures.length === 0 ? 0 : 1);
