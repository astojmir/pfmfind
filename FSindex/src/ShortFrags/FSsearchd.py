#!/usr/bin/env python

from ShortFrags.Expt import SearchServer
import sys, os, signal, os.path, string

# Based on Chad J. Schroeder's recipe: http://aspn.activestate.com/ASPN/Cookbook/Python/Recipe/278731 



class Log:
    """file like for writes with auto flush after each write
    to ensure that everything is logged, even during an
    unexpected exit."""
    def __init__(self, f):
        self.f = f
    def write(self, s):
        self.f.write(s)
        self.f.flush()

def read_config(serverfile):
    fp = file(serverfile, 'r')
    servers = []
    for line in fp:
        sp_line = string.split(line)
        host = sp_line[0]
        port = int(sp_line[1])
        workdir = sp_line[2]
        if len(sp_line) > 3:
            index = sp_line[3]
        else:
            index = ''
        servers.append((host, port, workdir, index))
    fp.close()
    return servers

def create_daemon(serverfile):
    """Disk And Execution MONitor (Daemon)

    Default daemon behaviors (they can be modified):
    1.) Ignore SIGHUP signals.
    2.) Default current working directory to the "/" directory.
    3.) Set the current file creation mode mask to 0.
    4.) Close all open files (0 to [SC_OPEN_MAX or 256]).
    5.) Redirect standard I/O streams to "/dev/null".

    Failed fork() calls will return a tuple: (errno, strerror).  This behavior
    can be modified to meet your program's needs.

    Resources:
    Advanced Programming in the Unix Environment: W. Richard Stevens
    Unix Network Programming (Volume 1): W. Richard Stevens
    http://www.erlenstar.demon.co.uk/unix/faq_2.html#SEC16
    """
    
    try:
        # Fork a child process so the parent can exit.  This will return control
        # to the command line or shell.  This is required so that the new process
        # is guaranteed not to be a process group leader.  We have this guarantee
        # because the process GID of the parent is inherited by the child, but
        # the child gets a new PID, making it impossible for its PID to equal its
        # PGID.
        pid = os.fork()
    except OSError, e:
        return((e.errno, e.strerror))     # ERROR (return a tuple)

    if (pid == 0):       # The first child.

        # Next we call os.setsid() to become the session leader of this new
        # session.  The process also becomes the process group leader of the
        # new process group.  Since a controlling terminal is associated with a
        # session, and this new session has not yet acquired a controlling
        # terminal our process now has no controlling terminal.  This shouldn't
        # fail, since we're guaranteed that the child is not a process group
        # leader.
        os.setsid()

        # When the first child terminates, all processes in the second child
        # are sent a SIGHUP, so it's ignored.
        signal.signal(signal.SIGHUP, signal.SIG_IGN)

        try:
            # Fork a second child to prevent zombies.  Since the first child is
            # a session leader without a controlling terminal, it's possible for
            # it to acquire one by opening a terminal in the future.  This second
            # fork guarantees that the child is no longer a session leader, thus
            # preventing the daemon from ever acquiring a controlling terminal.
            pid = os.fork()        # Fork a second child.
        except OSError, e:
            return((e.errno, e.strerror))  # ERROR (return a tuple)

        if (pid == 0):      # The second child.
            # Ensure that the daemon doesn't keep any directory in use.  Failure
            # to do this could make a filesystem unmountable.
            os.chdir("/")
            # Give the child complete control over permissions.
            os.umask(0)
        else:
            pidfile = os.path.join(os.path.split(serverfile)[0], "FSsearchd.pid")
            open(pidfile,'w').write("%d\n" % pid) # Write pid of the daemon
            os._exit(0)      # Exit parent (the first child) of the second child.
    else:
        os._exit(0)         # Exit parent of the first child.


    # Read the configuration file
    servers = read_config(serverfile)

    # Change to data directory
    datadir = os.path.expanduser(servers[0][2])
    logfile = os.path.join(datadir, "FSsearchd.log")
    os.chdir(datadir)

    # Close all open files.  Try the system configuration variable, SC_OPEN_MAX,
    # for the maximum number of open files to close.  If it doesn't exist, use
    # the default value (configurable).

    try:
        maxfd = os.sysconf("SC_OPEN_MAX")
    except (AttributeError, ValueError):
        maxfd = 256       # default maximum

    for fd in range(0, maxfd):
        try:
            os.close(fd)
        except OSError:   # ERROR (ignore)
            pass

    # Redirect the standard file descriptors to /dev/null and logfiles.
    os.open("/dev/null", os.O_RDONLY)                  # standard input  (0)
    os.open(logfile, os.O_CREAT|os.O_APPEND|os.O_RDWR) # standard output (1)
    os.open(logfile, os.O_CREAT|os.O_APPEND|os.O_RDWR) # standard error  (2)

    # Start the SearchServer 
    SrchS = SearchServer.SearchServer(servers)
    SrchS.start()


if __name__ == "__main__":
    create_daemon(sys.argv[1])
