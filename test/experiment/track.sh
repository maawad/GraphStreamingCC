top -b -Em -p $1 | awk -v pid=$1 '$1 ~ pid {print}'

