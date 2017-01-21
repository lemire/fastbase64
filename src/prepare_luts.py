def compress_mask(x):
    mask    = [-1] * 16
    index   = 0
    for i in xrange(16):
        bit = 1 << i
        if x & bit:
            mask[index] = i
            index += 1

    return mask


def prepare_popcnt_LUT():
    n = 65536
    for x in xrange(n):
        mask = compress_mask(x)
        s    = ', '.join('%2d' % x for x in mask)
        if x < n - 1:
            fmt = "    %s, // %02x"
        else:
            fmt = "    %s  // %02x"

        print fmt % (s, x)


def main():
    print "uint8_t compress_LUT[16 * 65536] __attribute__((aligned(4096))) = {"
    prepare_popcnt_LUT()
    print "};"

if __name__ == '__main__':
    main()
