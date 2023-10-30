.global _start

// Please keep the _start method and the input strings name ("input") as
// specified below
// For the rest, you are free to add and remove functions as you like,
// just make sure your code is clear, concise and well documented.

_start:
	// load input string address to r4
	ldr r4, =input
	// load LED port location to r5
	ldr r5, =0xff200000
	// prepare arguments for check_input
	mov r0, r4
	mov r1, #0
	mov r2, #0
	bl check_input
	// store return of check_input in r6
	mov r6, r2
	// prepare arguments for check_palindrom
	// put index of first char in r2 and index of last char in r3
	mov r2, r4
	add r3, r4, r6
	sub r3, r3, #1
	// put first char in r0
	ldrb r0, [r4]
	// put last char in r1
	ldrb r1, [r3]
	bl check_palindrom
	// prepare arguments for is_palindrome
	mov r0, r5
	bl is_palindrom
	b _exit

check_input:
	// load next char into r1
	ldrb r1, [r0], #1
	// check if end of string
	cmp r1, #0
	// increment char-count
	addne r2, #1
	// finish check_input when encountering end of phrase
	moveq r15, r14
	// check if char is a lowercase letter
	cmp r1, #0x61
	// if uppercase or number, branch to function
	blt fix_lowercase
	// branch back
	b check_input
	
check_palindrom:
	//check if whitespace
	cmp r0, #0x20
	//increment pointer if whitespace
	addeq r2, r2, #1
	//replace value with next char
	ldrb r0, [r2]
	cmp r1, #0x20
	subeq r3, r3, #1
	ldrb r1, [r3]
	// check if same character
	cmp r0, r1
	// move pointers toward center
	beq move_pointers
	//non-matching detected
	b is_no_palindrom
		
fix_lowercase:
	// check if uppercase letter
	cmp r1, #40
	// make lowercase
	addgt r1, r1, #32
	// store back to position
	strb r1, [r0, #-1]
	// go back to main function
	b check_input
	
	
move_pointers:
	cmp r2, r3
	bgt is_palindrom
	add r2, r2, #1
	sub r3, r3, #1
	ldrb r0, [r2]
	ldrb r1, [r3]
	b check_palindrom
	
is_palindrom:

	// ...01f to activate right-most LEDs
	ldr r0, =0x0000001f
	str r0, [r5]
	ldr r7, =palin
	mov r1, #0
	b print_result
		
is_no_palindrom:

	// ...3e0 to activate left-most LEDs
	ldr r0, =0x000003e0
	str r0, [r5]
	ldr r7, =nopalin
	mov r1, #0
	b print_result

print_result:
	ldrb r0, [r7], #1
	cmp r0, #0
	beq _exit
	add r1, r1, #1
	bl send_msg
	
send_msg:
	ldr r2, =0xff201000

	beq _exit
	str r0, [r2]
	b print_result

_exit:
	// Branch here for exit
	b .
	
.data
.align
	// This is the input you are supposed to check for a palindrom
	// You can modify the string during development, however you
	// are not allowed to change the name 'input'!
	
	input: .asciz "Grav ned den varg"
	// "a man 4 plan a canal p4nama"
	palin: .asciz "Palindrome detected"
	nopalin: .asciz "Not a palindrome"
.end