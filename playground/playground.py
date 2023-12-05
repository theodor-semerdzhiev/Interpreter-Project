import dis


def test():
    if False:
        num = 100
    def test(n):
        y = 10
        z = y
        def f(x):
            return n + x + y + z + num
        return f
    return test(10)

dis.dis(test)

print(test())