#!/bin/bash

if [ "$#" -ne 5 ]; then
    echo "Input format: ./create_infile.sh diseaseFile countriesFile input_dir numFilesPerDirectory numRecordsPerFile"
    exit 2
fi

dir=$3
rm -rf $dir;


numFilesPerDirectory=$4
numRecordsPerFile=$5

Names="Rosa
Jim
Anthony
Bruce
Christin
Jack
Jeremy
Elena
Stephen
Matt
Caroline
Suzan
Kathrine
Nadia
Rebeka
Carla
Lu
Valerio
Natasha"
name=($Names)
num_names=${#name[*]}

Surname="Smith
Vause
Chapman
Nichols
Reznikov
Gilbert
Salvatore
Fell
Forbs
Benett
Snow
Saltzman
Lockwood
Marshall
Smith
Davies
Brown
Mendez
Parker"
surname=($Surname)
num_surnames=${#surname[*]}

recordId=0

mkdir -p $3 #create dir input_dir

while read countryDir
do
    mkdir -p $3/$countryDir
    count=1
    while [ "$count" -le $numFilesPerDirectory ]  #create files in dir
    do
        let "day=$RANDOM%30 + 1"
        if [ $day -lt 10 ]
        then
          day="0$day"
        fi
        let "month=$RANDOM%12 +1"
        if [ $month -lt 10 ]
        then
          month="0$month"
        fi
        let "year=$RANDOM%20+ 2000"
        file_name="$day-$month-$year"
        touch $dir/$countryDir/$file_name
        let "count += 1"
    done

    for files in $dir/$countryDir/*; do
        numofRecords=$(< $files wc -l)
        for (( i = 0; i<$numRecordsPerFile-$numofRecords; i++ )); do
            let "recordId+=1"
            status="ENTRY"
            recordName="${name[$((RANDOM%num_names))]}"
            recordSurname="${surname[$((RANDOM%num_surnames))]}"
            let "age=$RANDOM%120 + 1"
            disease=$(shuf -r -n 1 $1)
            record="$recordId $status $recordName $recordSurname $disease $age"
            echo "$record" >> $files

            let "discharge=$RANDOM%100 +1"
            if [ $discharge -le 60 ]; then
                status="EXIT"
                record="$recordId $status $recordName $recordSurname $disease $age"
                randFile=$(ls $dir/$countryDir/ | shuf -n 1)
                randFileRecordCount=$(< $dir/$countryDir/$randFile wc -l)
                if [ $randFileRecordCount != $numRecordsPerFile ]; then
                    if [ $files == $dir/$countryDir/$randFile ]; then
                        let "i+=1"
                    fi
                    echo "$record" >> $dir/$countryDir/$randFile
                fi
            fi
        done
    done
done < $2