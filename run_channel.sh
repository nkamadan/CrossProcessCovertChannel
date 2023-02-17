rm -f rec.out;

for i in {0..4}; do
    taskset -c 0 ./receiver -f README.md  >> rec.out &
    taskset -c 2 ./sender -f README.md > /dev/null 2>&1 ;
    sleep 0.1;
    echo "\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n" >> rec.out
done 

cat rec.out
