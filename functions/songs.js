export async function onRequest(context) {
  const base = new URL(context.request.url);

  // songsフォルダ内のJSONを手動リスト（Cloudflareはfs使えない）
  const files = [
    "songs/canon.json",
    "songs/winter.json",
    "songs/boss/oni.json"
  ];

  const results = [];

  for (const file of files) {
    try {
      const url = new URL(file, base);
      const res = await fetch(url);
      const json = await res.json();
      results.push(json);
    } catch (e) {
      console.log("読み込み失敗:", file);
    }
  }

  return new Response(JSON.stringify(results), {
    headers: {
      "Content-Type": "application/json",
      "Access-Control-Allow-Origin": "*"
    }
  });
}