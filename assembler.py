import sys
import getopt

def parse(filename):
    try:
        f = open(filename, 'r')
        lines = f.readlines()
    except IOError, e:
        print 'IO error(%d): %s' % (e.errno, e.strerror)
    else:
        f.close()

def usage():
    print 'usage: %s <assembly file>' % sys.argv[0]

def main(argv):
    try:
        opts, args = getopt.getopt(argv, '')
    except getopt.GetoptError, e:
        usage()
        sys.exit(2)

    for opt, arg in opts:
        usage()
        sys.exit(2)

    if len(args) != 1:
        usage()
        sys.exit(2)

    parse(args[0])

if __name__ == '__main__':
    main(sys.argv[1:])
