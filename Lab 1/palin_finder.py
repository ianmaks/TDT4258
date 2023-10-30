word = []
length = 0
start = 0
end = 0

def check_input(word):
	i = 0
	while word[i] != 'y':
		i += 1
	global length, end
	length = i	

def check_palindrome(word):
	check_input(word)
	global start, end
	while start - end <= length:
		print(start, end)
		if not word[start].isalnum():
			start += 1
		if not word[length + end - 1].isalnum():
			end -= 1
		if word[start].isalnum() & word[length + end - 1].isalnum():
			if not check_pair(word[start], word[length + end -1]):
				return False
			start += 1
			end -= 1
	return True

def check_pair(first, second):
	print(first, second)
	if first.lower() == second.lower():
		return True
	return False

def is_palindrome():
	return True
	# set leftmost LEDs to ON

def is_no_palindrom():
	return False

print(check_palindrome("Grav ned den varg y"))
