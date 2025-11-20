#!/bin/bash
# script to initialize the directory of storage
createdir()
{
	local _tmpdir=$1
	local _ret;
	if [ -f $_tmpdir ];then
        	#echo "Error: $_tmpdir already exists and is not a direcotry"
        	echo "NOTFILE"
		return 1
	fi
	if [ ! -d $_tmpdir ];then
        	_ret=$(mkdir --mode 700 $_tmpdir)    
		if [ $? -ne 0 ];then
			echo "FAILED $_ret"
			return 1
		fi
	fi
	return 0
}
helpinfo()
{
    echo "usage: $0 datadir [mysql|sqlite]"
    echo "   for example $0 /home/chyd/dzfile"     
    exit 1
}
checkdir()
{
    local dirn=$1
    local dn
    local fn
    dn=$(dirname $dirn)
    fn=$(basename $dirn)
    if [ "$dn" = "/" ];then
        echo "$tn '$dirn' should be a second level directory, such as '/home/chyd/dzfile'"
        exit 1
    fi   
    if [ ! -d $dn ];then
        echo "The parent directory of $tn '$dirn' doesn't exist, please create it ( $dn ) firstly"
        exit 1
    fi
 
    if [ "$fn" != "dzfile" ];then
        echo "$tn '$dirn' should contain 'dzfile', such as '/home/chyd/dzfile'"
        exit 1
    fi 
}
checkdbtype()
{
    local dbtype=$1
    local d
    local isfound
    isfound=0
    for d in $supported_dbs;do
        #echo "$d"
        if [ $dbtype = $d ];then
            isfound=1
            break;
        fi
    done
    if [ $isfound -eq 0 ];then
        echo "#dbtype '$dbtype' is not supported, it should be one of '$supported_dbs'"
        exit
    fi
}

hint()
{
    local input
    echo "##The script will remove all the data from '$datadir',  '$dbtype'"
    echo -e "\033[5;31mIT IS VERY DANGEROUS!!!\033[0m"
    read -r -p "Are You Sure? [YES/NO] " input
    case $input in
        [yY][eE][sS]|[yY])
            echo "Confirmed"
            ;;
        *)
            echo "Exit..."
            exit 1
            ;;
    esac
}
sqlite_init()
{
    local srcdir
    local mbin
    scrdir=$(dirname $0)
    
    if [ ! -f ${scrdir}/meta.sql ];then
        echo "sqlite initiation script '${scrdir}/meta.sql' not exist"
        exit 1
    fi
    mbin=$(which sqlite3 2>/dev/null)
    if [ "x$mbin" = "x" ];then
        echo "Please check whether Sqlite3 client is installed properly "
        exit 1
    fi
    rm -rf $datadir/.dzs/meta.db
    sqlite3 -init ${scrdir}/meta.sql $datadir/.dzs/meta.db

}

#main
## initialize the directory of cloud config 
#mkdir -p /etc/dzfile
mkdir -p /var/log/dzfile/
mkdir -p /mnt/mm/
#chmod 644 /etc/dcfile/cloud.cfg

datadir=/home/chyd/dzfile
dbtype=sqlite
supported_dbs="sqlite mysql"
if [ $# -lt 1 ];then
    helpinfo
fi
if [ "$1" = "-h" -o "$1" = "--help" -o "$1" = "help" ];then
    helpinfo
fi
datadir=$1
checkdir $datadir 
isfound=0
if [ $# -ge 2 ];then
    dbtype=$2
    checkdbtype $dbtype
fi

hint
 
if [ ! -d $datadir ];then
    #echo "Error: $datadir is not a directory or does not exists"
	createdir $datadir
    [ $? -ne 0 ] && exit 1;
fi

#delete EVERYTHING in metadata
rm -rf $datadir/*
#delete other thing
createdir /var/log/dzfile

ret=$(createdir "$datadir/.dzs")
if [ "$ret" = "NOTFILE" -o "$ret" = "FAILED" ];then
	echo "Error: create dir $datadir/map ($ret)"
	createdir $datadir
    [ $? -ne 0 ] && exit 1;
fi

case $dbtype in
    "sqlite") 
        sqlite_init;
        ;;
    "mysql")
        mysql_init;
        ;;
esac

exit 0

