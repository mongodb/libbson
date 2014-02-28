AC_CANONICAL_HOST

os_win32=no
os_linux=no
os_freebsd=no
os_gnu=no

case "$host_os" in
    *-mingw*|*-*-cygwin*)
        os_win32=yes
        ;;
    *-*-*netbsd*)
        os_netbsd=yes
        ;;
    *-*-*freebsd*)
        os_freebsd=yes
        ;;
    *-*-*openbsd*)
        os_openbsd=yes
        ;;
    *-*-linux*)
        os_linux=yes
        os_gnu=yes
        ;;
    *-*-solaris*)
        os_solaris=yes
        ;;
    *-*-darwin*)
        os_darwin=yes
        ;;
    gnu*|k*bsd*-gnu*)
        os_gnu=yes
        ;;
    *)
        AC_MSG_WARN([*** Please add $host to configure.ac checks!])
        ;;
esac
