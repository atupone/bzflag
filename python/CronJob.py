from bzfsItf import bz_debugMessage
from bzfsItf import bz_getDebugLevel

class CronJob():
    inputJob = ''
    command  = ''
    minutes  = []
    hours    = []
    days     = []
    months   = []
    weekdays = []

    def __init__(self, job):
        self.setJob(job)

    def setJob(self, job):
        if not job.strip():
            return
        # parse the string we're given into five vectors of n ints and a command
        # note: this is rather expensive
        self.inputJob = job

        # first bust it up into tokens based on whitespace.
        # the first five are the timing values and the 'sixth through nth' is the command.
        toks = job.strip().split(sep=None, maxsplit=5)

        # hokey dokey.  now we have six strings and we need five arrays of ints and one string out of them.
        self.minutes  = self.parseTimeList(toks[0], 0, 59);
        self.hours    = self.parseTimeList(toks[1], 0, 23);
        self.days     = self.parseTimeList(toks[2], 1, 31);
        self.months   = self.parseTimeList(toks[3], 1, 12);
        self.weekdays = self.parseTimeList(toks[4], 0, 7);
        self.command  = toks[5];

        # sunday is both 7 and 0, make sure we have both or neither
        if 0 in self.weekdays and 7 not in self.weekdays:
            self.weekdays.push_back(7)
        elif 7 in self.weekdays and 0 not in self.weekdays:
            self.weekdays.push_back(0)

        # dump the list if we're debuggering
        if bz_getDebugLevel() >= 4:
            print('bzfscron: read job: '     + inputJob)
            print('bzfscron: job minutes: '  + vector_dump(self.minutes))
            print('bzfscron: job hours: '    + vector_dump(self.hours))
            print('bzfscron: job days: '     + vector_dump(self.days))
            print('bzfscron: job months: '   + vector_dump(self.months))
            print('bzfscron: job weekdays: ' + vector_dump(self.weekdays))
            print('bzfscron: job command: '  + self.command)

    def matches(self, n, h, d, m, w):
        # if we are supposed to execute now, return true, otherwise return false
        return (n in self.minutes) and (h in self.hours) and (d in self.days) \
                and (m in self.months) and (w in self.weekdays)

    def getCommand(self):
        return self.command

    def displayJob(self):
        return self.inputJob

    def isInVector(self, iv, x):
        return x in iv

    def parseTimeList(self, timeString, min, max):
        vi = []

        # First things first.  Find out if there's a periodicity and trim it off.
        pos    = timeString.find("/")
        period = 1

        if pos == -1:
            timeList = timeString
        else:
            period   = int(timeString[pos+1:])
            timeList = timeString[:pos]

        # Now tokenize on ","
        stage1 = timeList.split(',')

        # And for each token, blow up any "-" ranges and "*" ranges.
        for itr in stage1:
            if '*' in itr:
                bz_debugMessage(4, "bzfscron: exploding * range")
                vi += list(range(min, max + 1))
            else:
                pos = itr.find('-')
                if pos == -1:
                    bz_debugMessage(4, "bzfscron: using single int")
                    vi.append(int(itr))
                else:
                    bz_debugMessage(4, "bzfscron: exploding x-y range")
                    rmin = int(itr[:pos])
                    rmax = int(itr[pos + 1:])
                    if (rmin < min):
                        rmin = min
                    if (rmax > max):
                        rmax = max
                    vi += list(range(rmin, rmax + 1))

        # Remember that periodicity we got rid of earlier?  Now we need it.
        # Eliminate any elements which disagree with the periodicity.
        if period > 1:
            vp = []
            for itr2 in vi:
                if itr2 % period == 0:
                    vp.append(itr2)
            return vp
        else:
            return vi

# debug util func
def vector_dump(iv):
    tmp = '<'
    for itr in iv:
        tmp += ' {}'.format(itr)
    tmp += ' >'
    return tmp
