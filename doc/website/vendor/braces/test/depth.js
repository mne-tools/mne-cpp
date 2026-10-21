// SPDX-License-Identifier: MIT
// Regression test for GHSA-vfj7-8cjw-p6xm: deep nesting is rejected, not a stack overflow.
'use strict';

const assert = require('assert');
const braces = require('..');

const nested = depth => '{'.repeat(depth) + 'a,b' + '}'.repeat(depth);

// Ordinary patterns behave as in braces 3.0.3
assert.deepStrictEqual(braces('a/{b,c}/d'), ['a/(b|c)/d']);
assert.deepStrictEqual(braces('x{01..03}y', { expand: true }), ['x01y', 'x02y', 'x03y']);
assert.deepStrictEqual(braces('{a,{b,{c,d}}}', { expand: true }), ['a', 'b', 'c', 'd']);
assert.strictEqual(braces.expand(braces.parse(nested(128))).length, 2);

// Deeper patterns, and deep or cyclic ASTs passed in directly, throw a SyntaxError
assert.throws(() => braces(nested(129)), SyntaxError);
assert.throws(() => braces(nested(4999), { expand: true }), SyntaxError);
assert.throws(() => braces('('.repeat(4999) + 'a' + ')'.repeat(4999)), SyntaxError);
const deep = { type: 'root', nodes: [] };
let node = deep;
for (let i = 0; i < 12000; i++) {
  const child = { type: 'brace', nodes: [] };
  node.nodes.push(child);
  node = child;
}
const cyclic = { type: 'root', nodes: [] };
cyclic.nodes.push(cyclic);
for (const walker of ['compile', 'expand', 'stringify']) {
  assert.throws(() => braces[walker](deep), SyntaxError, walker);
  assert.throws(() => braces[walker](cyclic), SyntaxError, walker);
}
console.log('braces depth bound: ok');
