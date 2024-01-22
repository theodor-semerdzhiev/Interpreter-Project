#!bin/bash

NbOfTests=0
passed=0
failed=0
test_files=($(ls ./tests))


make clean
make
ite=0
for file in "${test_files[@]}"; do
    ./main.out ./tests/$file >/dev/null
    if [ $? -eq 0 ]; then
        ((passed++))
        echo "TEST $ite $file: PASSED"
    else
        echo "TEST $ite $file: FAILED"
    fi
    ((ite++))
    ((NbOfTests++))
done

echo "PASSED $passed / $NbOfTests tests"