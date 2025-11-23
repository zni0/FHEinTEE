for i in {1..5}
do
	A=$(( RANDOM % 100 + 1 ))
	B=$(( RANDOM % 100 + 1 ))

	./client $A $B
	if [ $? -ne 0 ]; then
		echo "Failed for $A, $B"
		exit 1
	fi
done
