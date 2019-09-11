for i in /home/wx50/ece551/mp_miniproject/testcases/*
do
    valgrind ./myshell < $i
done
