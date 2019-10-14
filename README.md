# ProjectLunarPhase

A simple web server framework in C++, created after being much annoyed by the strange syntax of Python - See oy other repository [FlaskSite](https://github.com/Edgaru089/FlaskSite).

Part of the BNDS technological course "Database"

## This branch:
SFNUL is hard to compile on a less-powerful Linux device (like a 1-core VPS or something) because of the dependency of compiling Botan as a cryptography interface. For that reason, I switched to SFML for cross-platform networking, which lacks less-important features like IPv6 or TLS.

## Acknowledgements

[SFNUL](https://github.com/binary1248/SFNUL) (Included Part, Modified) - under the Mozilla Public License 2.0

[mysql-cpp](https://github.com/bskari/mysql-cpp) (Included) - under the MIT License (internally uses the MySQL Connector/C under the GNU GPLv2) (Modified to get rid of Boost)

[sanae.png](https://github.com/Edgaru089/ProjectLunarPhase/blob/master/RunDir/static/sanae.png) - part of the ThCRP by 
kaoru (see infomation on [THBWiki](https://thwiki.cc/Alphes%E9%A3%8E%E7%AB%8B%E7%BB%98%E7%B4%A0%E6%9D%90))
