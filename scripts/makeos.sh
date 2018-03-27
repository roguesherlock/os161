alias makeos='function _makeos {
    ASST=""
    case "$1" in 
    "asst0")
        ASST="DUMBVM"
        ;;
    "asst1")
        ASST="ASST1"
        ;;
    "asst2")
        ASST="ASST2"
        ;;
    "asst3")
        ASST="ASST3"
        ;;
    *)
        echo "No such assignment exists"
        return 1
    esac
 
    BUILDDIR=$HOME"/Developer/os161/kern/compile/"$ASST
    ROOTDIR=$HOME"/os161/root"
 
    cd $BUILDDIR
    bmake && bmake install && cd $ROOTDIR && echo "Done!"
}; _makeos'

alias ss='function _ss {
    ROOTDIR=$HOME"/os161/root"
    cd $ROOTDIR
    sys161 kernel
}; _ss'

alias ssw='function _ssw {
    ROOTDIR=$HOME"/os161/root"
    cd $ROOTDIR
    sys161 -w kernel
}; _ssw'

alias sg='function _sg {
    ROOTDIR=$HOME"/os161/root"
    cd $ROOTDIR
    os161-gdb -x $ROOTDIR/gdb_commands.txt kernel
    
}; _sg'


# Add this to your ~/.bashrc or source it.
