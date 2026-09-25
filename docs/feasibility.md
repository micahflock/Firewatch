# Feasibility: how to start the whole fireplace with one action

The goal is to wake the TCL 32S335 Roku TV, get the YouTube fireplace playing, and start the LED glow, ideally with one action. Four approaches were evaluated, simplest first.

## Verdicts

| Approach | Works? | Actions | Notes |
|---|---|---|---|
| 1. Smart plug on everything, no code | **No** | 1 | Roku TVs don't restart a streaming app after power loss. The TV comes back to the Home screen or a TV input at best. |
| 2. Roku ECP over Wi-Fi (network API) | **Yes, recommended** | 1 | Needs *Fast TV start* on and *Control by mobile apps* enabled. |
| 3. IR blaster | Fallback only | 1 | Only needed to wake a TV whose network sleeps in standby. It can't launch a specific video, so ECP still does that part. |
| 4. Follow the TV's power (ESP32 watches the TV over ECP) | **Yes, recommended** | 1 | The Roku remote's power button becomes the single action, and turning the TV off puts the glow out. |

The firmware does both 2 and 4. Turning the TV on by any means (remote, Roku voice, the Roku phone app, an Alexa or Google skill) starts the glow and launches the fire video. Turning the scene on from Firewatch (web page, phone shortcut, button) wakes the TV. Turning either one off turns off both. In practice that's one action to start and one to stop, and you don't need a new remote.

---

## 1. Smart plug only (the no-code option)

**The idea:** leave the TV playing the YouTube fire and cut its power with a smart plug. When power comes back, hope the TV resumes where it left off. Put the LEDs on the same plug, running a controller with a built-in fire effect (for example WLED's "Fire 2012" as its boot preset).

**Why it doesn't work:**

- A Roku TV's power-on behavior is set in **Settings → System → Power → Power On**. It offers **Home** or a **TV input** (HDMI, AV, Antenna) and nothing else ([TCL support](https://support.tcl.com/rokutv-setup-configuration/02-set-what-your-tcl-roku-tv-displays-upon-power-on)). There's no "last app" option, and a streaming channel like YouTube isn't an input.
- Cutting AC forces a **cold boot**, not the instant fast-start wake. Roku doesn't document whether the TV powers itself up after an outage or waits in standby. Either way it ends up on Home or an input, not on YouTube playing your video.
- Hard-cutting power every day also skips standby, which is when Roku installs its overnight updates.

**The LED half of this does work on its own** (an off-the-shelf WLED controller with a fire preset on a smart plug). But you'd still need 3+ remote presses to get YouTube back to the video, so it fails the "one or two actions" bar.

**Two-minute test to confirm on your unit** (worth doing, since it's free):
1. Play the fire video. Pull the TV's plug, wait 30 s, and plug it back in.
2. Note whether the TV turns on by itself, and what it shows (Home, an HDMI input, or YouTube).
3. If it somehow shows the video playing, this approach works for the TV side and you could skip the ECP half. That would be surprising.

**Partial variant, also rejected:** put only the LED controller on a smart plug and have it wake the TV over ECP when it gets power. Turning *on* is then one action. But turning *off* cuts the controller's power instantly, so the lights snap off with no fade and the TV stays on. You'd need two actions to shut down. The always-on controller in the recommended design avoids this.

## 2. Roku ECP (External Control Protocol)

ECP is Roku's official LAN API: plain HTTP on port 8060 ([Roku docs](https://developer.roku.com/dev/docs/external-control-api)). Every call the firmware needs:

| Purpose | Call | Restrictions |
|---|---|---|
| Discover the TV | SSDP `M-SEARCH`, `ST: roku:ecp` | None |
| Is the screen on? | `GET /query/device-info` → `<power-mode>` (`PowerOn` = on; `DisplayOff` and similar = standby) | None |
| What's in the foreground? | `GET /query/active-app` → `<app id="837">YouTube</app>` | None |
| Wake the TV | `POST /keypress/PowerOn` (plus a Wake-on-LAN packet) | Needs *Control by mobile apps* enabled |
| Sleep the TV | `POST /keypress/PowerOff` | Same |
| Start the fire | `POST /launch/837?contentId=<videoId>&mediaType=movie` | None |

**Requirements on the TV:**

- **Fast TV start = On** (Settings → System → Power). This keeps the TV's Wi-Fi awake in standby so `PowerOn` can reach it. Without it the network is off in standby, and nothing on the LAN can wake the TV. Roku's menu warns this raises standby power use.
- **Control by mobile apps = Enabled/Default/Permissive, not Limited or Disabled** (Settings → System → Advanced system settings → Control by mobile apps → Network access). Recent Roku OS releases added a *Limited* mode that rejects keypresses with `ECP command not allowed in limited mode`, and some units now ship with it on ([Home Assistant issue](https://github.com/home-assistant/home-assistant.io/issues/36240)). The firmware detects this error and shows the fix on its status page.
- **Wake-on-LAN:** the 32S335 has an Ethernet port, and its `device-info` lists the Wi-Fi and Ethernet MACs. The firmware sends a magic packet to both before `PowerOn`. It costs nothing, but I don't rely on it: with fast start on, `PowerOn` alone works. Home Assistant's Roku integration uses the same keypress for power-on.
- **Input selection:** no separate step is needed. Launching a channel over ECP takes the TV off whatever HDMI or antenna input it's on. Set *Power On* to **Home** so a cold boot never sits on an empty HDMI input.

**Risks and mitigations:**

- **The YouTube deep link is widely used but not a published YouTube contract.** `launch/837?contentId=<id>` has worked for years and is what Home Assistant users rely on. If YouTube ever changes it, the fallback is for the ESP32 to launch YouTube without a content ID and step through the menus with keypresses. That's fragile, so it isn't built in.
- **YouTube may already be running after a power-on, but parked on its home screen.** So on a real power-on edge the firmware always sends the launch, and checks `active-app` to confirm and retry (up to 3 tries, 15 s apart). If only the ESP32 rebooted while the fire was already playing, it doesn't relaunch.
- **Long sessions:** Roku's *Auto power savings* turns the TV off after 4 hours without remote input. Turn that off, or keep it as a sleep timer; the lights follow the TV either way. YouTube can also pause and ask "still watching?". Use a long (8–12 h) video. The web page's **Restart video** button forces a fresh launch.
- **Security:** ECP and Firewatch's own endpoints are unauthenticated on your LAN. That's normal for home media gear, but don't port-forward either one.

## 3. IR blaster fallback

**Only needed if** fast TV start can't be used, for example because you want the lowest possible standby power. In that case the TV is invisible to the network in standby.

- TCL Roku TVs accept NEC IR codes with **discrete** power commands: `0x57E316E9` = on, `0x57E318E7` = off (the 32-bit form used by IRremoteESP8266). These are community-sourced ([code list](https://gist.github.com/DDRBoxman/c68b68e41a47338496ba8cbd1bb5d20e)). Discrete codes are safe to repeat, unlike a power toggle.
- IR can't deep-link to a video. The firmware runs a **hybrid**: IR wakes the TV, the TV joins Wi-Fi within about 10–20 s, and then ECP launches the fire as usual. With `IR_LED_PIN` set, the IR power-on is sent alongside every ECP wake.
- Hardware: an IR LED, one NPN transistor, and two resistors (about $2), with the LED on a thin lead aimed at the IR receiver window on the TV's front bottom edge. See [hardware.md](hardware.md#optional-ir-blaster).
- Following the TV's power still works without fast start: the TV is unreachable in standby and reachable once it's on, and the firmware treats three missed polls in a row as "off".

## 4. Following the TV's power (the actual single action)

The ESP32 polls `device-info` every 2 s. That's two tiny HTTP GETs, which is negligible load for the TV. The policy lives in [`scene.cpp`](../firmware/lib/firecore/src/scene.cpp) and is covered by host unit tests:

```
TV turns on (remote, voice, app)  ─► glow fades in; launch fire video; confirm via active-app
Firewatch "on" (web/shortcut/btn) ─► glow fades in now; WoL + PowerOn (+IR), retry every 6 s
                                     for up to 60 s; launch once the TV reports PowerOn
TV turns off                      ─► glow fades out
Firewatch "off"                   ─► glow fades out; PowerOff to the TV
One dropped poll                  ─► ignored (3 in a row = off)
ESP32 reboots under a running fire ─► glow resumes, video left alone
```

If the TV never wakes, the glow stays on by itself. The status page shows `TV did not wake; lights only`, and if you then turn the TV on with the remote it joins the scene.

## What was verified vs. what needs your TV

- **Verified here:** the firmware compiles for ESP32 (RGBW, RGB, and IR builds). 25 host unit tests pass, covering the scene policy, ECP response parsing, dithering accuracy, the power limiter, and fire-field behavior. The mounting STLs are watertight meshes exported from the parametric source. The fire animation was rendered offline ([preview](img/preview.gif)).
- **Needs your hardware:** the power-restore test above, whether your Roku OS build defaults to *Limited* mode, how long the TV takes to wake, and color and brightness tuning against your actual firebox. Sooty brick reflects much less than paint, so expect to raise brightness. [install.md](install.md) has a 5-minute `curl` checklist that proves the ECP path before you solder anything.
