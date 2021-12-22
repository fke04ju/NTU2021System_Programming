#!/bin/bash

while getopts "m:n:l:" command
do
    case $command in
        m)
            host_num=$OPTARG
            ;;
        n)
            player_num=$OPTARG
            ;;
        l)
            lucky_num=$OPTARG
            ;;
    esac
done

if [ $host_num -lt 1 -o $host_num -gt 10 ];then
    echo "Host number is out of range."
    exit
fi
if [ $player_num -lt 8 -o $player_num -gt 12 ];then
    echo "Player number is out of range."
    exit
fi
if [ $lucky_num -lt 0 -o $lucky_num -gt 1000 ];then
    echo "Lucky number is out of range."
    exit
fi

declare -a available_host
declare -a score
declare -i available_host_num
available_host_num=$host_num

for fn in $(seq 0 $host_num)
do
    fname="fifo_$fn.tmp"
    mkfifo "$fname"
    exec {fd}<> fifo_${fn}.tmp
    available_host+=([$fn]=0)
done

for sc in $(seq 0 $player_num)
do
    score+=([$sc]=0)
done

for hn in $(seq 1 $host_num)
do
    ./host -d 0 -l $lucky_num -m $hn &
done

for ((a=1 ; a<=$player_num ; a=a+1));do
    for ((b=$a+1 ; b<=$player_num ; b=b+1));do
        for ((c=$b+1 ; c<=$player_num ; c=c+1));do
            for ((d=$c+1 ; d<=$player_num ; d=d+1));do
                for ((e=$d+1 ; e<=$player_num ; e=e+1));do
                    for ((f=$e+1 ; f<=$player_num ; f=f+1));do
                        for ((g=$f+1 ; g<=$player_num ; g=g+1));do
                            for ((h=$g+1 ; h<=$player_num ; h=h+1));do
                                while [ $available_host_num -eq 0 ];do
                                    ((available_host_num++))
                                    read host_id < "fifo_0.tmp"
                                    for p in $(seq 1 8);do
                                        read player_id point < "fifo_0.tmp"
                                        score[$player_id]=$((score[$player_id]+$point))
                                    done
                                    available_host[$host_id]=0
                                done
                                for ((av=1 ; av<=$host_num ; av=av+1));do
                                    if [ ${available_host[$av]} -eq 0 ];then
                                        available_host[$av]=1
                                        echo "$a $b $c $d $e $f $g $h" > "fifo_$av.tmp"
                                        ((available_host_num--))
                                        break
                                    fi
                                done
                            done
                        done
                    done
                done
            done
        done
    done
done

while [ $available_host_num -ne $host_num ]
do
    ((available_host_num++))
    read host_id < "fifo_0.tmp"
    for p in $(seq 1 8);do
        read player_id point < "fifo_0.tmp"
        score[$player_id]=$((score[$player_id]+$point))
    done
    available_host[$host_id]=0
done

for i in $(seq 1 $host_num)
do
    echo "-1 -1 -1 -1 -1 -1 -1 -1" > "fifo_$i.tmp"
done

declare -i cur_max
declare -i top
declare -i max_who
declare -i rank
declare -a rank_arr
top=2147483647
rank=1
max_who=0

for rk in $(seq 1 $player_num)
do
    rank_arr+=([$rk]=0)
done

for i in $(seq 1 $player_num);do
    cur_max=0
    for j in $(seq 1 $player_num);do
        if [ $cur_max -le ${score[$j]} ] && [ ${rank_arr[$j]} -eq 0 ];then
            cur_max=${score[$j]}
            max_who=$j
        fi
    done
    if [ $cur_max -lt $top ];then
        top=$cur_max
        rank=$i
    fi
    rank_arr[$max_who]=$rank
done

for p in $(seq 1 $player_num)
do
    echo "$p ${rank_arr[$p]}"
done

for i in $(seq 0 $host_num)
do
    exec {fd}<&-
done

for fname in $(seq 0 $host_num)
do
    rm "fifo_$fname.tmp"
done

wait