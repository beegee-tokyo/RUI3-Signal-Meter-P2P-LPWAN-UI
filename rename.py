import sys
import shutil
import os
import json
import zipfile
from pathlib import Path

# print("+++++++++++++++++++++++++")
# print("+++++++++++++++++++++++++")
# print("Run rename.py")
# print("+++++++++++++++++++++++++")
# print("+++++++++++++++++++++++++")

try:
	with open('./.vscode/arduino.json') as f:
		data = json.load(f)
	version = data['version']
except:
	version = '0.0.0'

# print("Version " + version)

try:
	with open('./.vscode/arduino.json') as f:
		data = json.load(f)
	source_file = './build/'+ data['sketch'] + '.hex'
	bin_name = data['sketch'] + '.bin'
	nrf_zip_file = data['sketch'] + '.zip'
except:
	source_file = '*.hex'
	bin_name = '*.bin'
	nrf_zip_file = '*.zip'

try:
	with open('./.vscode/arduino.json') as f:
		data = json.load(f)
	board_type = data['board']
except:
	board_type = 'rak'

try:
	with open('./.vscode/arduino.json') as f:
		data = json.load(f)
	project_name = data['project']
except:
	board_type = 'RUI3'

# print("Source " + source_file)
# print("Binary " + bin_name)
# print("ZIP " + nrf_zip_file)
# print("Board " + board_type)

# Specify the source file, the destination for the copy, and the new name
destination_directory = './generated/'
new_file_name = project_name+'_V'+version+'.hex'
new_zip_name = project_name+'_V'+version+'.zip'

if not os.path.exists(destination_directory):
	try:
		os.makedirs(destination_directory)
	except:
		print('Cannot create '+destination_directory)
	

if os.path.isfile(destination_directory+new_file_name):
	try:
		os.remove(destination_directory+new_file_name)
	except:
		print('Cannot delete '+destination_directory+new_file_name)
	# finally:
	# 	print('Delete '+destination_directory+new_file_name)

if os.path.isfile(destination_directory+new_zip_name):
	try:
		os.remove(destination_directory+new_zip_name)
	except:
		print('Cannot delete '+destination_directory+new_zip_name)
	# finally:
	# 	print('Delete '+destination_directory+new_zip_name)

if os.path.isfile('./build/'+new_zip_name):
	try:
		os.remove('./build/'+new_zip_name)
	except:
		print('Cannot delete '+'./build/'+new_zip_name)
	# finally:
	# 	print('Delete '+'./build/'+new_zip_name)

if os.path.isfile('./generated/'+new_zip_name):
	try:
		os.remove('./generated/'+new_zip_name)
	except:
		print('Cannot delete '+'./generated/'+new_zip_name)
	# finally:
	# 	print('Delete '+'./generated/'+new_zip_name)

# Copy the files
if board_type.find('rak4631') != -1:
	try:
		shutil.copy2(source_file, destination_directory)
	except:
		print('Cannot copy '+source_file +' to '+destination_directory)
# Get the base name of the source file
base_name = os.path.basename(source_file)

# print("Base name " + base_name)

# Construct the paths to the copied file and the new file name
copied_file = os.path.join(destination_directory, base_name)
new_file = os.path.join(destination_directory, new_file_name)
zip_name = project_name+'_V'+version+'.zip'
hex_name = project_name+'_V'+version+'.hex'

# print("Copied file " + copied_file)
# print("Base name " + new_file)
# print("ZIP name " + zip_name)
# print("ZIP content " + bin_name) 

# Create ZIP file for WisToolBox
if board_type.find('rak4631') != -1:
	try:
		os.chdir("./build")
	except:
		print('Cannot change dir to ./build')

	try:
		zipfile.ZipFile(zip_name, mode='w').write(bin_name)
	except:
		print('Cannot zip '+bin_name +' to '+zip_name)

	os.chdir("../")

	try:
		shutil.copy2("./build/"+zip_name, destination_directory)
	except:
		print('Cannot copy '+"./build/"+zip_name +' to '+destination_directory)

	# Rename the file
	try:
		os.rename(copied_file, new_file)
	except:
		print('Cannot rename '+copied_file +' to '+new_file)
else:
	# Copy the files
	try:
		shutil.copy2("./build/"+nrf_zip_file, "./generated/"+zip_name)
	except:
		print('RAK4631 Cannot copy '+nrf_zip_file +' to ./generated/'+zip_name)

	try:
		shutil.copy2(source_file, "./generated/"+hex_name)
	except:
		print('RAK4631 Cannot copy '+source_file +' to ./generated/'+hex_name)

print("++++++++++++++++++++++++++++++++++++++++++++++++++")
print("Generated distribution files")
print(*Path("./generated/").iterdir(), sep="\n")
print("++++++++++++++++++++++++++++++++++++++++++++++++++")
