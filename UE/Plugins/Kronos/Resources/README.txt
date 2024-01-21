Thank you for purchasing Kronos Matchmaking!

If you have any questions regarding the plugin, feel free to reach out!
 - Discord: horizongamesroland
 - Email: horizongames.contact@gmail.com

If you are interested in how the plugin works, I recommend checking out the header files of the classes. They are well documented!
Here is a quick summary of how matchmaking works behind the scenes:

 1. A KronosMatchmakingPolicy object is created by the KronosMatchmakingManager.
 2. StartMatchmaking() is called on the matchmaking policy. Parameters will be validated and a KronosSearchPass object will be created for the matchmaking policy.
 3. BeginMatchmaking() will be called on the matchmaking policy, and the first function in the matchmaking chain will be executed.
 4. The matchmaking policy will continue to execute a chain of async functions. This function chain is known as a "matchmaking pass".
 5. If the "matchmaking pass" is unsuccessful, RestartMatchmaking() will be called and the chain will start again. This cycle will continue as long as a pass is successful, or we hit a search limit.
 6. Once matchmaking is complete, the KronosOnlineSession will handle the rest. It will check if any additional operations are needed, such as opening a listen server or connecting to a host.

If you are satisfied with the product, please consider leaving a rating or a review on the marketplace! It helps a lot!
Thank you, and good luck with your future game development endeavours! :)
