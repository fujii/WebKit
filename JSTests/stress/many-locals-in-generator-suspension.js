//@ memoryHog!
//@ slow!

var source = '(function* generatorFunction() {';

for (var i = 0; i < 65536; ++i) {
    source += `
        var v_${i} = Number.NaN * 0 | (
            Object.defineProperty(Math.pow, "length", {
                value: -0,
                writable: true
            }),
            Math.pow.length
        );

        if ('' == ({_: 5e-324})._ !== true) {
        }
    `;
}

source += `
    yield;
})`;

var generatorFunction = eval(source);
var generator = generatorFunction();

i.__defineGetter__('__proto__', () => {});
generator.next({ g: 5 }, { g: 5 });
