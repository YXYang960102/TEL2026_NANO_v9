# TEL2026 NANO v9

依最新總集重新生成。

DR1=A3、DR2=A2、DW1=A1、MA=D11、MC=D3、MB=D12、MD=D7、SA=D6、SB=D5、SC=D4、SD=A4。

DR1（接 Mega D22，發射時 100ms HIGH）LOW→HIGH 且 SA 或 SB 至少一個 LOW 才 need_pieces +1。上一顆還沒離開 SA（need_pieces 還是 1）時再收到 DR1，當作卡球不再加，need_pieces 最多 1，避免卡球鬆開後一次送兩顆。
SA 由 LOW 轉 HIGH（球離開 SA）就 need_pieces -1，一次發射只送一顆，就算下一顆緊跟在後也一樣。need_pieces > 0 超過 1.5 秒都沒有球離開，自動歸 0，MA 停止全速推。
發射中（從 DR1 到最後一顆離開 SA 後再 0.2 秒 CLEAR_MS）：MA 全速、MB 停止，讓頂端的球完全出去，第二顆不會被往上頂而卡住；之後才恢復 MA 慢速、MB 往上送。
DR2（接 Mega A15，停用時 HIGH）HIGH 超過 50ms 後 need_pieces=0 且停止全部馬達；DR2 LOW 超過 50ms 解除。開機時直接依 DR2 決定是否停用。
DW1（接 Mega A14）每顆球送一次 100ms HIGH：啟用中 SA/SB 由都 HIGH 變成任一 LOW 時送出；由停用轉啟用時若 SA/SB 已有球也補送一次。每次發射結束（CLEAR_MS 結束或 1.5 秒逾時）時頂端還有球也補送一次，因為下一顆緊跟在後時 SA/SB 可能一直沒空過，Mega 發射後會等下一個脈衝才再射（全自動、半自動，2026-11-14）。
MD 在 SD=HIGH 時以 正轉 1800us 0.6秒 / 反轉 1200us 0.3秒 / 停 1500us 0.05秒 循環，SD=LOW 或停用時 1500us 停止。
全程使用 millis()，不使用 delay()。
