import click

import p4.vocabs.boot, p4.dictionary, p4.vocabs.debug
import test.utils
words = p4.vocabs.boot.ALL + p4.vocabs.debug.ALL

@click.command()
@click.option("--vocabs", "-v", multiple=True)
@click.option("--preloads", "-p", multiple=True)
@click.option("--bootstrap/--no-bootstrap", default=True)
@click.option("--debug/--no-debug", default=False)
def cli(vocabs, preloads, bootstrap, debug):
    words = []
    for vocab in vocabs:
        match vocab.lower():
            case "boot": words.extend(p4.vocabs.boot.ALL)
            case "debug": words.extend(p4.vocabs.debug.ALL)

    vm = test.utils.create_vm(words)
    if bootstrap:
        #if "p4/bootstrap/control.f" not in preloads: preloads = ("p4/bootstrap/control.f", *preloads)
        if "p4/bootstrap/dummy.f" not in preloads: preloads = ("p4/bootstrap/dummy.f", *preloads)
    contents = []
    for preload in preloads:
        click.echo(f"Preloading {preload}")
        with open(preload) as x: contents.extend(x.readlines())
    # Can only buffer input text once; old contents are cleared.
    vm.io.buffer_text("\n".join(contents))
    test.utils.next(vm, "INTERP")
    vm.debug = debug
    # TODO: run until buffer text is empty, then enable debug mode.
    vm.run()

if __name__ == "__main__":
    cli()