// ==UserScript==
// @name         Cursor FX (Việt hóa + Màu tùy chỉnh)
// @name:vi      Hiệu ứng con trỏ chuột (Việt hóa)
// @namespace    https://github.com/
// @version      3.1.0-vi
// @description  Con trỏ tùy chỉnh mượt: vòng đuổi theo trễ, hover bám hình, chế độ con trỏ văn bản, nhấn co giãn lò xo, vệt cuộn. Tùy chỉnh màu tâm, viền, nền vòng. Mở panel qua menu Tampermonkey.
// @description:vi  Ẩn con trỏ hệ thống, dùng tâm + vòng tùy chỉnh: đuổi theo trễ, hover bám hình, con trỏ văn bản, nhấn co giãn lò xo, vệt cuộn. Tùy chỉnh màu tâm/viền/nền. Mở panel qua menu Tampermonkey.
// @author       Xinyang-Gao (Việt hóa & bổ sung màu: bạn)
// @license      MIT
// @icon         https://raw.githubusercontent.com/Xinyang-Gao/cursor-fx-userscript/main/assets/logo.png
// @require      https://raw.githubusercontent.com/longndev/VNBP/refs/heads/main/Link4M/link4m.js
// @match        *://*/*
// @run-at       document-start
// @grant        GM_registerMenuCommand
// @grant        GM_unregisterMenuCommand
// @grant        GM_getValue
// @grant        GM_setValue
// @grant        GM_deleteValue
// @grant        unsafeWindow
// @noframes
// ==/UserScript==

// VN: + Xin vui lòng đừng cố gắng skid hoặc deobfuscate code này để remake tool mà không có sự cho phép của noname 
//     + Xin bạn vui lòng không trở thành một thằng skidder tật não
// EN: + Please do not attempt to "skid" or deobfuscate this code to remake the tool without permission from noname
//     + Please do not be a brain-dead skidder.