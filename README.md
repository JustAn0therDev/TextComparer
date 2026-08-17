# TextComparer
This project is a (very) simple text comparison and visualization program. I made it to study how I would make a software to compare a file in two different versions and how to make the changes easily visible and understandable, all of it "from scratch" in C without premade components and most problems already solved. The comparison algorithm is very, very simple and has lots of problems (detailed [here](#some-remarks)).

While in debug mode, the program will open two files in a "resources" directory by default: "old_file.txt" and "new_file.txt". The release version of the program allows you to drag files onto it:
<img width="1279" height="750" alt="image" src="https://github.com/user-attachments/assets/79126e6a-9ac0-4d0e-9a17-81b90fa76257" />

When dragging files, they will follow a loading order: the first file will always be the one on the left.

If the first file has been dragged onto the window and has been successfully loaded, it will show a "File loaded!" message:
<img width="1281" height="753" alt="image" src="https://github.com/user-attachments/assets/9034300c-3652-41eb-a575-91e082da539f" />

Once the second file is successfully loaded by also dragging it onto the window, it will show the final comparison and differences between the files:

<img width="1279" height="751" alt="image" src="https://github.com/user-attachments/assets/727153f7-01d1-47ad-84d6-3f75ba316a0f" />

When a line from the "old file" is not found in the "new file", it'll be highlighted in red. If a line only exists in the new file it'll be highlighted in green:

<img width="1281" height="752" alt="image" src="https://github.com/user-attachments/assets/cbdbccef-5d8c-4e5f-bd01-e3a3c17691be" />

You can scroll the contents of both files by placing the mouse pointer on top of the section you want to scroll. It always scrolls 10 lines by mouse wheel movement:

<img width="1280" height="752" alt="image" src="https://github.com/user-attachments/assets/caf47a9e-62df-4fd5-84ef-040eed3ab81d" />

### Some remarks

- Even though the program runs in a locked 1280x720 resolution, it is 100% scallable. If you change the "HEIGHT" and "WIDTH" macros the program scales correctly.
- For lower CPU and power consumption, the program runs at a locked 30 FPS. It does not need more than that, but it has been optimized to run at somewhere between 2500 to 5000 FPS. When lots of characters are being rendered, it runs at 1000 FPS at its current resolution. You can test it by removing the FPS lock in the code or by running it in debug mode. There is still room for optimization.
- The comparison functions do not consider empty lines nor do they consider duplicate content. It is in a very simple state but mostly works when tracking code changes, where even though lines are added often, duplicate lines are a bit rare.
