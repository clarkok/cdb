#!/usr/bin/node

'use strict';

let rows = parseInt(process.argv[2], 10);

console.log('insert into student values');

function randomStr(len) {
    const ALPHABET = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwzyx0123456789';
    let ret = '';

    while (len--) {
        ret += ALPHABET[(Math.random() * ALPHABET.length) | 0];
    }

    return ret;
}

let rows_list = [];

for (let i = 0; i < rows; ++i) {
    rows_list.push('(' +
            [
                '\'' + i + '\'',
                '\'' + randomStr(8) + '\'',
                (Math.random() * 40) | 0,
                (Math.random() > 0.5) ? '\'M\'' : '\'F\'',
                (Math.random() * 5.0)
            ].join(',')
            + ')');
}

console.log(rows_list.join(',\n'));

console.log(';');
