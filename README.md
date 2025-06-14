
# TuneScape   
### **4.8.5 - experimental**   
In version 4.8.5, a new feature has been added that allows exporting IceCast favorite stations to an M3U playlist.   

### **4.8.4 - experimental**   
For now you can export an M3U playlist and listen to it on VLC or other compatible apps with the M3U standard. The second thing is that you can import playlists to TuneScape. For now, only RadioBrowser. 

I highly encourage you to download the M3U playlist from https://www.radio-browser.info/ and choose your category or TAGS or everything else and load it to TuneScape.   
### **4.7.4**   
The main icon is changed. The icon is from [iconduck.com](https://iconduck.com/icons/105034/radio) and is for [GPL v2](https://www.gnu.org/licenses/old-licenses/gpl-2.0.html) licence.   
### **4.7.3**   
Version 4.7.3 ... more functionality. A crash radio station is handled.   
![connect](https://github.com/user-attachments/assets/30052936-4b9e-48df-8350-4f2017ca6cc1)   

### **4.7.0**   
Version 4.7.0 brings us the countries choosing.   
![world](https://github.com/user-attachments/assets/4423ba6c-f5ba-44af-9730-96f3fca2619d)   

### **4.6.0**   
We get a new feature with version 4.6.0. Miniplayer that shows for you only minimal useful information and can stay on top of other windows if you want.   
<br/>
![light50x50](https://github.com/user-attachments/assets/041ff162-260b-43ea-b563-3646ac1d119d)
![dark50x50](https://github.com/user-attachments/assets/ccdf692a-926e-43c2-9658-de1d917330a2)   
<br/>
### **4.5.0**   
Version 4.5.0 brings a new look for TuneScape. The appearance of the light and dark themes is more elegant, and its behavior is idealy the same.
I prepared architecture to add more themes in the future too.   
<br/>
![light](https://github.com/user-attachments/assets/e23a5170-d8b5-45c2-89bf-d0e08c4b70de)   
<br/>
![dark](https://github.com/user-attachments/assets/d7b539e4-45c7-4b34-927f-99192f303826)   
<br/>
### **4.4.0**   
In version 4.4.0 is added export and import functionality for the favorite playlists and a simple settings/menu button.   
<br/>
![playlists](https://github.com/user-attachments/assets/757570af-1877-4645-852f-dc7aa1ee3aec)   
### **4.3.1**   
In version 4.3.1, I resigned from creating independent widgets (FFT) to create some easily multiplatform integration.   
For now I use QGraphicsView, and there are some problems in Linux. I think I will resolved this problem in the future.   
I have fought seriously with FFT. There were very uncommon problems with buffers and streams!!!. The tests took a lot of time, but now it looks great.   
### **A new release, 4.2.1, with audio frequency spectrum.**   
![fft](https://github.com/user-attachments/assets/6bcc87d3-4990-4d28-a480-d4dfcd38b903)   
### **You can test the new beta version 4.1.0 with the tray radio lists.**   
![smallmenu](https://github.com/user-attachments/assets/0ebbd10b-7ee7-41e3-a502-f719af5ce6a5)   

### **A new release, 4.0.0, is now available. Qt 6.7.0**   

### Expansion of features:    
- Add a record icon to the tray menu.
- Add a menu for a radio-browser station to tray the menu.   
### **The new branch for Linux development's name is "linux".**   
![image](https://github.com/user-attachments/assets/626865e2-fe7a-4238-8e25-b281d37d5a4e)   


![tray1](https://github.com/grzesiekkedzior/TuneScape/assets/23739158/a5697f87-8b78-46a0-bdc7-20f93afb1f69)
![tray2](https://github.com/grzesiekkedzior/TuneScape/assets/23739158/38610f42-4f10-4fe3-a3a0-c07b8bbd7356)
   


This is an open-source internet radio app that provides access to a wide range of radio stations. It leverages a database of radio stations from the website https://www.radio-browser.info/ to offer a diverse selection of online radio channels for all users.

**TuneScape** for now is being tested on the Windows platform.

[![Download TuneScape](https://a.fsdn.com/con/app/sf-download-button)](https://sourceforge.net/projects/tunescape/files/latest/download)   

## 🎵 Support TuneScape

If you enjoy using TuneScape and would like to support its development, you can make a donation via PayPal.  
Every contribution helps maintain the project, add new features, and improve performance.  

### 💰 Donate via PayPal  
You can support TuneScape by sending a donation through PayPal:  
[![Donate via PayPal](https://img.shields.io/badge/Donate%20via%20PayPal-00457C?logo=paypal&logoColor=white&style=for-the-badge)](https://www.paypal.com/donate/?hosted_button_id=MW4VMJ8YHSZF2)

Or simply scan the QR code below:  

![Kod QR](https://github.com/user-attachments/assets/a9c86292-1220-4e7e-b7b2-6e7415075220)

### 🙌 Other Ways to Support  
If you can't donate, you can still help by:  
- ⭐ Starring the project on GitHub to increase its visibility.  
- 📝 Providing feedback and suggestions to improve TuneScape.  
- 📢 Sharing the app with friends who love internet radio.  

Thank you for your support! 🎶❤️  


# License
**TuneScape** is released under the GPLv3 license.  
**The dark theme** "Combinear" with modification is from [Qt Style Sheets Templates](https://qss-stock.devsecstudio.com/) and is under an MIT license.

# How it works

You can add a station to favorites when the station is playing and the same when you want to remove it.

The meta data is read even when you pause playing the radio so that titles change fluently.

**TuneScape** informs you when connection will be lost and when the connect refreshes all automatically.

**TuneScape** uses for every start all the newest radio databases. In that case, sometimes it can be a little slow, but only at the start.

If you search for a radio station that you are interested in and you want to listen to later, add it to your favorite bookmark.

When you search for a station and list it and click other bookmarks on then you lose all your search stations.

# Contributing

**An App by the People, for the People**

**Your Contribution** is Invaluable: Regardless of who you are or what you bring to the table, your ideas and skills are irreplaceable. This is your opportunity to be a part of creating something truly exceptional.

**For All of Us:** Our application is a creation of many people with diverse backgrounds and skills, designed with everyone in mind. No matter where you come from, our initiative aims to provide value for each and every user.

**Choose Your Area:** Whether you're a developer, designer or have other talents, there's a place for you here.

**Early Stage:** Our application is in its initial phase, which means that this is the moment when your ideas and involvement can have the greatest impact. It's an opportunity to shape the project right from the beginning.
