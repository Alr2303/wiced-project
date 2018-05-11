usage() { 
    echo "stlink utility is needed to use this script."
    echo "Mac OS =>  $ brew install stlink "
    echo "Usage: $0 [-t <string> boot | dct | app ] [-f <string>]"  1>&2; 
    exit 1; 
}

while getopts ":t:f:" o; do
    case "${o}" in
        t)
            t=${OPTARG}
            ;;
        f)
            f=${OPTARG}
            ;;
        *)
            usage
            ;;
    esac
done
shift $((OPTIND-1))

if [ -z "${t}" ] || [ -z "${f}" ]; then
    usage
fi

echo target : ${t}
echo binary : ${f}

target="0x0800C000"

case "${t}" in
    boot)
        target="0x08000000"
        ;;
    dct)
        target="0x08004000"
        ;;
    app)
        target="0x0800C000"
        ;;
    *)
        usage
        ;;
esac
        
echo ${target}
st-flash write ${f} ${target}
