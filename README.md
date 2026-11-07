# TEL2026 NANO v9

依最新總集重新生成。

DR1=A3、DR2=A2、DW1=A1、MA=D11、MC=D3、MB=D12、MD=D7、SA=D6、SB=D5、SC=D4、SD=A4。

DR1（接 Mega D22，發射時 100ms HIGH）LOW→HIGH 且 SA 或 SB 至少一個 LOW 才 need_pieces +1。
SA 確認 LOW 後，轉 HIGH 並維持 0.5 秒才 need_pieces -1。
DR2（接 Mega D15，停用時 HIGH）HIGH 超過 50ms 後 need_pieces=0 且停止全部馬達；DR2 LOW 超過 50ms 解除。開機時直接依 DR2 決定是否停用。
DW1（接 Mega D14）每顆球送一次 100ms HIGH：啟用中 SA/SB 由都 HIGH 變成任一 LOW 時送出；由停用轉啟用時若 SA/SB 已有球也補送一次。
MD 在 SD=HIGH 時以 1700us 0.6秒 / 1500us 1秒循環，SD=LOW 時 1500us。
全程使用 millis()，不使用 delay()。
