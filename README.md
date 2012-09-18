
# Wineing

Wineing enables NxCore (running on Wine) to talk to Linux
applications. It links against Wine and ZeroMq (linux) to build a
compatibility layer between two processes, one running NxCore and
another one running a linux application. It aims to be fail-proof even
under high loads. Google Protobuf is used to generate the wiring protocol.

The code is heavily documented and follows doxygen documentation
schema, see for example *Makefile* or *src/main/c/wineing.win.cc*.

The last section describes how to run the java WineingExampleClient application.

TODO: write tests...

Tested with:

-   Google Protocol buffer 2.4.1-2 (C++)
-   ZeroMQ 2.2.0-2 (C version)
-   Wine 1.5.6
-   Check (unit test library for c)

## Running Wineing

Running Wineing is easy. Once compiled the binary is available in
`<wineing-root>/target/<wineing-version>/`. To invoke `wineing.exe`
type

    $ noglob <wineing-root>/target/<wineing-version>/wineing.exe \
                    --cchan=<fqcn> \
                    --mchan=<fqcn> \
                    [--schan=<fqcn>] \
                    [--tape-root=<dir>]

The `noglob` option is only relevant to zsh users. It disables
globbing. The `lib` folder in `<wineing-version>` is required by
`wineing.exe`. If it is not available `wineing.exe` fails to run.

TODO: just a sketch. explain thoroughly...

-   run wineing
-   run client
-   client starts command channel (see example java client for controlflow)
-   client sends connection requ  est
-   client can now connect to control response and market data channel
-   client can request live market data

Client must implement the Google Protocol specs. See compiling google
protocol buffers.

## Installation

Installation from source is fairly easy.

**Note:** Wineing was build and tested on Arch-Linux only. Most of the
instructions below should also apply to other *nix systems though.

### Getting the dependencies

Most dependencies can be installed through the distribution's package
manager. In fact it is the recommended approach but for
zeromq. Instead it is suggested to compile zeromq manually and link
Wineing against the binary. It is the recommented approach because the
user is in control of the ZMQ version in use, especially when running
wineing and client on different archs.

    $ yaourt -S protobuf
    $ yaourt -S wine

Wineing is currently built aginst zeromq 2.2.0. To build zmq type

    $ wget http://download.zeromq.org/zeromq-2.2.0.tar.gz
    $ tar zxvf zeromq-2.2.0.tar.gz
    $ cd zeromq-2.2.0
    $ ./autogen.sh
    $ ./configure --prefix=/home/you/tmp
    $ make && make install

### Building Wineing

First get the sources

      $ git clone git@bitbucket.org:instilled/wineing.git

In the wineing root directory type

    $ make

This will build wineing with debug flags enabled. To compile wineing
with maximal optimization type

    $ make release

In any case `make` will run the tests (bähh that's not yet true)
against the binary. Finally the binary is placed to
`wineing\target`. To clean the build hit

    $ make clean

**Note:** Building wineing will put the NxCore shared library in the
target dir. If the binary is moved elsewhere make sure to also copy
the `lib/` dir.

### Generating Google Protobuf for your language

For C++, Java, and Python (and other supported languages) things are
easy, see [protobuf-gen]. For C++ e.g. type

    $ protoc -I=$SRC_DIR --cpp_out=$DST_DIR \
                   $SRC_DIR/WineingMarketDataProto.proto \
                   $SRC_DIR/WineingCtrlProto.proto

where `$SRC_DIR` points to
`<wineing-root>/src/main/resources/protobuf` and `$DST_DIR` to any
directory of choice. If generating java sources and `$DST_DIR` ends in
`.jar` sources are packed in a jar.

[protobuf-gen]: https://developers.google.com/protocol-buffers/docs/tutorials

## WineingExampleClient

WineingJ is an example client to connect to Wineing. It shall outline the basic
workflow between Wineing server (aka Wineing) and this client application.

TODO: ZMQ's java binding requires a build against the native library on each
platform. That is, we will have to provide a Java binding for each platform.
The java binding tests against the major and minor build versions of the native
library which will be different on each arch (TODO: true?)

#### Dependencies

 - zeromq 2.2.0
 - jzmq
 - Google Protocol Buffers 2.4.1

### Running WineingExampleClient

The jzmq jar has to be installed in the local repository prior to build/run
WineingJ (see below) otherwise the build will fail. Also it is required that
`protoc` is on the `PATH`. The easiest way to get `protoc` on OS x is to
install it through e.g. mac ports `ports install protobuf-java`. Finally to
compile (and package) WineingExampleClient type

    $ mvn package

That's it. Try

    $ java -jar target/WineingJ-<VERSION>.jar --help

### Java ZMQ bindings installation

It is currently not supported to build the native libraries while
building the project. The dependencies have to be installed in the
local maven repository (usually `~/.m2/repository`) manually. It is
assumed that `jzmq-<ver>.jar` is shipped with the native libraries
embedded in the jar, see [Repacking jzmq](#repacking). The native
library must reside in
`<JAR>/NATIVE/${os.arch}/${os.name}/libjzmq.[so|dylib|dll]` within the
jar.

To install jzmq in the local maven repository type

    $ mvn install:install-file  -Dfile=ext/libzmq/jzmq-2.2.0.jar \
                                -DgroupId=com.jzmq \
                                -DartifactId=jzmq \
                                -Dversion=2.2.0 \
                                -Dpackaging=jar

    $ mvn install:install-file  -Dfile=ext/libzmq/jzmq-2.2.0-sources.jar \
                                -DgroupId=com.jzmq \
                                -DartifactId=jzmq \
                                -Dversion=2.2.0 \
                                -Dpackaging=jar \
                                -Dclassifier=sources

### Repacking jzmq

JZMQ was built against zeromq on each platform and the native library bindings
added to the jzmq jar to assure jzmq's major/minor version match the native
library version. To build jzmq against a specific zeromq library, assuming zeromq
was installed in `/tmp/zeromq-2.2.0-bin`, type

    $ ./autogen.sh
    $ ./configure --prefix=/tmp/jzmq-2.2.0-bin \
                  --with-zeromq=/tmp/zeromq-2.2.0-bin

The final `zmq.jar` was renamed to `jzmq-<version>.jar` and the native libraries
added to the jar `<JAR>/NATIVE/${os.arch}/${os.name}/libjzmq.[so|dylib|dll]`.
Note that ${os.arch} and ${os.name} must match the output of Java's
`System.getProperty("os.[arch|name]")`.


## Misc

This section will be removed in the future. It's a bunch of
information that helped during development but is not anymore
necessary or helpful.

See
http://check.sourceforge.net/doc/check_html/check_2.html#SEC3 for a
(incomplete) list of c test frameworks.

### Unit testing

**Check**
- manual http://check.sourceforge.net/doc/check_html/index.
- pros
  - uses fork()
- cons
  - documentation

**CUnit**
- http://cunit.sourceforge.net/
- pros
  - very easy!
  - similar in concept to JUnit
- cons
  - does not use fork() to protect address space (see
    http://check.sourceforge.net/doc/check_html/check_2.html)

### Logging

**log4c**
- http://log4c.sourceforge.net/
- pros
  - similar to log4j
- cons
  - no documentation!

**c-logger**
* pros
  * they say they're similar to [[liblogger]] but I haven't found any
    documentation
* cons
  * no docs
  * dead?

**liblogger**
* pros
  * well documented with lots of examples
  * zero performance overhead when logging is disables (compiler macros)
  * very easy to use
  * available through yourt
  * apache license 2.0
* cons
    * dead? last update dates back to 2011-09-17

### Build systems

**SCons**
** Make**

## Notes

### Static vs. dynamic linking
- building libs
  - http://codingfreak.blogspot.ch/2009/12/creating-and-using-shared-libraries-in.html
  - http://codingfreak.blogspot.ch/2010/01/creating-and-using-static-libraries-in.html
  - http://www.cs.duke.edu/~ola/courses/programming/libraries.html
**performance related**
- http://stackoverflow.com/questions/1993390/static-linking-vs-dynamic-linking

### Google Protocol buffer in Wine code
Protobuf depends on pthread. Pthread is only the interface to
threading in Linux/Unix. To determine the pthread library in use gconf
like so:

    getconf GNU_LIBPTHREAD_VERSION.

Ever since glibc 2.3.2 Linux ships with NPTL. For further details on pthreads
read the man page 'man pthreads'.

I was investigating if pthreads could cause a problem in combination
with a winelib linked binary. In july 2009 J. McKenzie wrote:

'Wine pthread is[..] gone [..] because Linux and
UNIX both do not support it anymore. If you really, really, really
need it,get Wine 1.0.1 for now.'
(see http://wiki.jswindle.com/index.php/WineLib)

Wine does not use pthreads. Read
http://www.winehq.org/site/docs/winedev-guide/threading for more
details on the topic.

Pthreads and Google Protocol Buffers might indeed be a pitfall,
especially because related issues exist (see
http://code.google.com/p/protobuf/issues/detail?id=248).

False: [Also when compiling wineg++ returns with some unresolved
comilation problems. Instead I've generated the protobuf and compiled
it into a shared lib and then linked that lib with the final Wine
executable (that might not solve the threading issues outlined
above).

To build and link NxCoreConfigProto:

    $ gcc -c -fPIC -lprotobuf -lpthread NxCoreConfigProto.pb.cc
    $ gcc -shared -o libNxCoreConfigEventProto.so NxCoreConfigProto.pb.o

NOTE: Google Protocol buffer depends on libpthread]

Instead of building a shared lib we complie the protocol buffer
interface as a shared object file and link it into the executable.

    $ g++ -fPIC -c src/gen/c/NxCoreConfigProto.pb.cc
    $ wineg++ -mwindows -o target/zmq-server.exe zmq_server.o \
       NxCoreConfigProto.pb.o -lzmq -lodbc32 -lole32 -loleaut32 -lwinspool \
       -lodbccp32 -luuid -lpthread -lprotobuf

## Winemaker
With Winemaker it is easy to generate a Makefile to build a winelib
enabled application or library. Modifications to the Makefile are
required in our case (I don't know yet why we have to disable cygwin
and fall back on navtie Mingw headers and libraries.):

* pass -mno-cygwin to gcc (or winegcc respectively)

See http://www.delorie.com/howto/cygwin/mno-cygwin-howto.html for a
thorough explanation on what the option does. The default compilation
mode is "cygwin", and the compiler by default looks for header files
that are Cygwin specific and also links in the Cygwin runtime
libraries. With the -mno-cygwin option instead gcc looks for native
Mingw32 headers and links in the Mingw32 runtime libraries.

## Resources
