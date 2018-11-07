import glad.lang.c


def get_generator(name, spec):
    _langs = [
        glad.lang.c,
    ]

    for lang in _langs:
        gen, loader = lang.get_generator(name, spec)
        if gen is not None:
            return gen, loader
    return None, None
